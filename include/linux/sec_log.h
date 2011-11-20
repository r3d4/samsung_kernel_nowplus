#ifndef _SEC_LOG_H_
#define _SEC_LOG_H_

#define SEC_LOG_BUF_FLAG_SIZE		(4 * 1024)
#define SEC_LOG_BUF_DATA_SIZE		(1 << CONFIG_LOG_BUF_SHIFT)
#define SEC_LOG_BUF_SIZE		(SEC_LOG_BUF_FLAG_SIZE + SEC_LOG_BUF_DATA_SIZE)
#define SEC_LOG_BUF_START		(0x94b80000 - SEC_LOG_BUF_SIZE)
#define SEC_LOG_BUF_MAGIC		0x404C4F47	/* @LOG */

struct struct_plat_log_mark {
  u32 special_mark_1;
  u32 special_mark_2;
  u32 special_mark_3;
  u32 special_mark_4;
  void *p_main;
  void *p_radio;
  void *p_events;
  void *p_system;
};

struct struct_kernel_log_mark {
  u32 special_mark_1;
  u32 special_mark_2;
  u32 special_mark_3;
  u32 special_mark_4;
  void *p__log_buf;
};

struct struct_frame_buf_mark {
  u32 special_mark_1;
  u32 special_mark_2;
  u32 special_mark_3;
  u32 special_mark_4;
  void *p_fb;
  u32 resX;
  u32 resY;
  u32 bpp;
  u32 frames;
};

struct struct_marks_ver_mark {
  u32 special_mark_1;
  u32 special_mark_2;
  u32 special_mark_3;
  u32 special_mark_4;
  u32 log_mark_version;
  u32 framebuffer_mark_version;
  void * this;
  u32 first_size;
  u32 first_start_addr;
  u32 second_size;
  u32 second_start_addr;
};

struct sec_log_buf {
	unsigned int *flag;
	unsigned int *count;
	char *data;
};

#endif
