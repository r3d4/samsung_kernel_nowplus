/* rwsem-spinlock.c: R/W semaphores: contention handling functions for
 * generic spinlock implementation
 *
 * Copyright (c) 2001   David Howells (dhowells@redhat.com).
 * - Derived partially from idea by Andrea Arcangeli <andrea@suse.de>
 * - Derived also from comments by Linus
 */
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/slab.h>

// klaatu semaphore debug
#ifdef CONFIG_SAMSUNG_SEMAPHORE_LOGGING

enum {
	READ_SEM,
	WRITE_SEM
};

#define RWSEMAPHORE_LOG_MAX 100
struct rwsem_debug{
	struct list_head list;
	struct rw_semaphore *sem;
	struct task_struct *task;
	pid_t pid;
	int cpu;
	int direction;
	/* char comm[TASK_COMM_LEN]; */
};

/* rwsemaphore logging */
struct rwsem_debug rwsem_debug_free_head;
struct rwsem_debug rwsem_debug_done_head;
int rwsem_debug_free_head_cnt;
int rwsem_debug_done_head_cnt;
int rwsem_debug_init = 0;
spinlock_t rwsem_debug_lock;

/* rwsemaphore logging */
void debug_rwsemaphore_init(void)
{
	int i = 0;
	struct rwsem_debug *rwsem_debug = NULL;

	spin_lock_init(&rwsem_debug_lock);
	rwsem_debug_free_head_cnt = 0;
	rwsem_debug_done_head_cnt = 0;

	/* initialize list head of sem_debug */
	INIT_LIST_HEAD(&rwsem_debug_free_head.list);
	INIT_LIST_HEAD(&rwsem_debug_done_head.list);

	for (i = 0; i < RWSEMAPHORE_LOG_MAX; i++) {
		/* malloc semaphore */
		rwsem_debug = kmalloc(sizeof(struct rwsem_debug), GFP_KERNEL);
		/* add list */
		list_add(&rwsem_debug->list, &rwsem_debug_free_head.list);
		rwsem_debug_free_head_cnt++;
	}

	rwsem_debug_init = 1;
}

void debug_rwsemaphore_down_log(struct rw_semaphore *sem, int dir)
{
	struct list_head *tmp;
	struct rwsem_debug *sem_dbg;
	unsigned long flags;

	if (!rwsem_debug_init)
		return;

	spin_lock_irqsave(&rwsem_debug_lock, flags);
	list_for_each(tmp, &rwsem_debug_free_head.list) {
		sem_dbg = list_entry(tmp, struct rwsem_debug, list);
		sem_dbg->task = current;
		sem_dbg->sem = sem;
		/* strcpy(sem_dbg->comm,current->group_leader->comm); */
		sem_dbg->pid = current->pid;
		sem_dbg->cpu = smp_processor_id();
		sem_dbg->direction = dir;
		list_del(&sem_dbg->list);
		list_add(&sem_dbg->list, &rwsem_debug_done_head.list);
		rwsem_debug_free_head_cnt--;
		rwsem_debug_done_head_cnt++;
		break;
	}
	spin_unlock_irqrestore(&rwsem_debug_lock, flags);
}

void debug_rwsemaphore_up_log(struct rw_semaphore *sem)
{
	struct list_head *tmp;
	struct rwsem_debug *sem_dbg;
	unsigned long flags;

	if (!rwsem_debug_init)
		return;

	spin_lock_irqsave(&rwsem_debug_lock, flags);
	list_for_each(tmp, &rwsem_debug_done_head.list) {
		sem_dbg = list_entry(tmp, struct rwsem_debug, list);
		if (sem_dbg->sem == sem && sem_dbg->pid == current->pid) {
			list_del(&sem_dbg->list);
			list_add(&sem_dbg->list, &rwsem_debug_free_head.list);
			rwsem_debug_free_head_cnt++;
			rwsem_debug_done_head_cnt--;
			break;
		}
	}
	spin_unlock_irqrestore(&rwsem_debug_lock, flags);
}

#define debug_rwsemaphore_down_read_log(x) \
	debug_rwsemaphore_down_log(x,READ_SEM)
#define debug_rwsemaphore_down_write_log(x) \
	debug_rwsemaphore_down_log(x,WRITE_SEM)

#endif /* CONFIG_SAMSUNG_SEMAPHORE_LOGGING */

struct rwsem_waiter {
	struct list_head list;
	struct task_struct *task;
	unsigned int flags;
#define RWSEM_WAITING_FOR_READ	0x00000001
#define RWSEM_WAITING_FOR_WRITE	0x00000002
};

int rwsem_is_locked(struct rw_semaphore *sem)
{
	int ret = 1;
	unsigned long flags;

	if (spin_trylock_irqsave(&sem->wait_lock, flags)) {
		ret = (sem->activity != 0);
		spin_unlock_irqrestore(&sem->wait_lock, flags);
	}
	return ret;
}
EXPORT_SYMBOL(rwsem_is_locked);

/*
 * initialise the semaphore
 */
void __init_rwsem(struct rw_semaphore *sem, const char *name,
		  struct lock_class_key *key)
{
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	/*
	 * Make sure we are not reinitializing a held semaphore:
	 */
	debug_check_no_locks_freed((void *)sem, sizeof(*sem));
	lockdep_init_map(&sem->dep_map, name, key, 0);
#endif
	sem->activity = 0;
	spin_lock_init(&sem->wait_lock);
	INIT_LIST_HEAD(&sem->wait_list);
}
EXPORT_SYMBOL(__init_rwsem);

/*
 * handle the lock release when processes blocked on it that can now run
 * - if we come here, then:
 *   - the 'active count' _reached_ zero
 *   - the 'waiting count' is non-zero
 * - the spinlock must be held by the caller
 * - woken process blocks are discarded from the list after having task zeroed
 * - writers are only woken if wakewrite is non-zero
 */
static inline struct rw_semaphore *
__rwsem_do_wake(struct rw_semaphore *sem, int wakewrite)
{
	struct rwsem_waiter *waiter;
	struct task_struct *tsk;
	int woken;

	waiter = list_entry(sem->wait_list.next, struct rwsem_waiter, list);

	if (!wakewrite) {
		if (waiter->flags & RWSEM_WAITING_FOR_WRITE)
			goto out;
		goto dont_wake_writers;
	}

	/* if we are allowed to wake writers try to grant a single write lock
	 * if there's a writer at the front of the queue
	 * - we leave the 'waiting count' incremented to signify potential
	 *   contention
	 */
	if (waiter->flags & RWSEM_WAITING_FOR_WRITE) {
		sem->activity = -1;
		list_del(&waiter->list);
		tsk = waiter->task;
		/* Don't touch waiter after ->task has been NULLed */
		smp_mb();
		waiter->task = NULL;
		wake_up_process(tsk);
		put_task_struct(tsk);
		goto out;
	}

	/* grant an infinite number of read locks to the front of the queue */
 dont_wake_writers:
	woken = 0;
	while (waiter->flags & RWSEM_WAITING_FOR_READ) {
		struct list_head *next = waiter->list.next;

		list_del(&waiter->list);
		tsk = waiter->task;
		smp_mb();
		waiter->task = NULL;
		wake_up_process(tsk);
		put_task_struct(tsk);
		woken++;
		if (list_empty(&sem->wait_list))
			break;
		waiter = list_entry(next, struct rwsem_waiter, list);
	}

	sem->activity += woken;

 out:
	return sem;
}

/*
 * wake a single writer
 */
static inline struct rw_semaphore *
__rwsem_wake_one_writer(struct rw_semaphore *sem)
{
	struct rwsem_waiter *waiter;
	struct task_struct *tsk;

	sem->activity = -1;

	waiter = list_entry(sem->wait_list.next, struct rwsem_waiter, list);
	list_del(&waiter->list);

	tsk = waiter->task;
	smp_mb();
	waiter->task = NULL;
	wake_up_process(tsk);
	put_task_struct(tsk);
	return sem;
}

/*
 * get a read lock on the semaphore
 */
void __sched __down_read(struct rw_semaphore *sem)
{
	struct rwsem_waiter waiter;
	struct task_struct *tsk;
	unsigned long flags;

	spin_lock_irqsave(&sem->wait_lock, flags);

#ifdef CONFIG_SAMSUNG_SEMAPHORE_LOGGING
	debug_rwsemaphore_down_read_log(sem);
#endif
	if (sem->activity >= 0 && list_empty(&sem->wait_list)) {
		/* granted */
		sem->activity++;
		spin_unlock_irqrestore(&sem->wait_lock, flags);
		goto out;
	}

	tsk = current;
	set_task_state(tsk, TASK_UNINTERRUPTIBLE);

	/* set up my own style of waitqueue */
	waiter.task = tsk;
	waiter.flags = RWSEM_WAITING_FOR_READ;
	get_task_struct(tsk);

	list_add_tail(&waiter.list, &sem->wait_list);

	/* we don't need to touch the semaphore struct anymore */
	spin_unlock_irqrestore(&sem->wait_lock, flags);

	/* wait to be given the lock */
	for (;;) {
		if (!waiter.task)
			break;
		schedule();
		set_task_state(tsk, TASK_UNINTERRUPTIBLE);
	}

	tsk->state = TASK_RUNNING;
 out:
	;
}

/*
 * trylock for reading -- returns 1 if successful, 0 if contention
 */
int __down_read_trylock(struct rw_semaphore *sem)
{
	unsigned long flags;
	int ret = 0;


	spin_lock_irqsave(&sem->wait_lock, flags);

	if (sem->activity >= 0 && list_empty(&sem->wait_list)) {
		/* granted */
		sem->activity++;
		ret = 1;
#ifdef CONFIG_SAMSUNG_SEMAPHORE_LOGGING
		debug_rwsemaphore_down_read_log(sem);
#endif
	}

	spin_unlock_irqrestore(&sem->wait_lock, flags);

	return ret;
}

/*
 * get a write lock on the semaphore
 * - we increment the waiting count anyway to indicate an exclusive lock
 */
void __sched __down_write_nested(struct rw_semaphore *sem, int subclass)
{
	struct rwsem_waiter waiter;
	struct task_struct *tsk;
	unsigned long flags;

	spin_lock_irqsave(&sem->wait_lock, flags);

#ifdef CONFIG_SAMSUNG_SEMAPHORE_LOGGING
	debug_rwsemaphore_down_write_log(sem);
#endif
	if (sem->activity == 0 && list_empty(&sem->wait_list)) {
		/* granted */
		sem->activity = -1;
		spin_unlock_irqrestore(&sem->wait_lock, flags);
		goto out;
	}

	tsk = current;
	set_task_state(tsk, TASK_UNINTERRUPTIBLE);

	/* set up my own style of waitqueue */
	waiter.task = tsk;
	waiter.flags = RWSEM_WAITING_FOR_WRITE;
	get_task_struct(tsk);

	list_add_tail(&waiter.list, &sem->wait_list);

	/* we don't need to touch the semaphore struct anymore */
	spin_unlock_irqrestore(&sem->wait_lock, flags);

	/* wait to be given the lock */
	for (;;) {
		if (!waiter.task)
			break;
		schedule();
		set_task_state(tsk, TASK_UNINTERRUPTIBLE);
	}

	tsk->state = TASK_RUNNING;
 out:
	;
}

void __sched __down_write(struct rw_semaphore *sem)
{
	__down_write_nested(sem, 0);
}

/*
 * trylock for writing -- returns 1 if successful, 0 if contention
 */
int __down_write_trylock(struct rw_semaphore *sem)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&sem->wait_lock, flags);

	if (sem->activity == 0 && list_empty(&sem->wait_list)) {
		/* granted */
		sem->activity = -1;
		ret = 1;
#ifdef CONFIG_SAMSUNG_SEMAPHORE_LOGGING
		debug_rwsemaphore_down_write_log(sem);
#endif
	}

	spin_unlock_irqrestore(&sem->wait_lock, flags);

	return ret;
}

/*
 * release a read lock on the semaphore
 */
void __up_read(struct rw_semaphore *sem)
{
	unsigned long flags;

	spin_lock_irqsave(&sem->wait_lock, flags);

	if (--sem->activity == 0 && !list_empty(&sem->wait_list))
		sem = __rwsem_wake_one_writer(sem);

#ifdef CONFIG_SAMSUNG_SEMAPHORE_LOGGING
	debug_rwsemaphore_up_log(sem);
#endif
	spin_unlock_irqrestore(&sem->wait_lock, flags);
}

/*
 * release a write lock on the semaphore
 */
void __up_write(struct rw_semaphore *sem)
{
	unsigned long flags;

	spin_lock_irqsave(&sem->wait_lock, flags);

	sem->activity = 0;
	if (!list_empty(&sem->wait_list))
		sem = __rwsem_do_wake(sem, 1);

#ifdef CONFIG_SAMSUNG_SEMAPHORE_LOGGING
	debug_rwsemaphore_up_log(sem);
#endif
	spin_unlock_irqrestore(&sem->wait_lock, flags);
}

/*
 * downgrade a write lock into a read lock
 * - just wake up any readers at the front of the queue
 */
void __downgrade_write(struct rw_semaphore *sem)
{
	unsigned long flags;

	spin_lock_irqsave(&sem->wait_lock, flags);

	sem->activity = 1;
	if (!list_empty(&sem->wait_list))
		sem = __rwsem_do_wake(sem, 0);

#ifdef CONFIG_SAMSUNG_SEMAPHORE_LOGGING
	debug_rwsemaphore_up_log(sem);
	debug_rwsemaphore_down_read_log(sem);
#endif
	spin_unlock_irqrestore(&sem->wait_lock, flags);
}

