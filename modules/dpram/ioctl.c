#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
  

/* ioctl command definitions. */
#if 1
#define IOC_MZ_MAGIC					('o')
#define DPRAM_PHONE_POWON				_IO(IOC_MZ_MAGIC, 0xd0)
#define DPRAM_PHONEIMG_LOAD				_IO(IOC_MZ_MAGIC, 0xd1)
#define DPRAM_NVDATA_LOAD				_IO(IOC_MZ_MAGIC, 0xd2)
#define DPRAM_PHONE_BOOTSTART			_IO(IOC_MZ_MAGIC, 0xd3)

#define IOC_SEC_MAGIC            (0xf0)
#define DPRAM_PHONE_ON           _IO(IOC_SEC_MAGIC, 0xc0)
#define DPRAM_PHONE_OFF          _IO(IOC_SEC_MAGIC, 0xc1)
#define DPRAM_PHONE_GETSTATUS    _IOR(IOC_SEC_MAGIC, 0xc2, unsigned int)
#define DPRAM_PHONE_MDUMP        _IO(IOC_SEC_MAGIC, 0xc3)
#define DPRAM_PHONE_BATTERY      _IO(IOC_SEC_MAGIC, 0xc4)
#define DPRAM_PHONE_RESET        _IO(IOC_SEC_MAGIC, 0xc5)
#define DPRAM_PHONE_RAMDUMP_ON    _IO(IOC_SEC_MAGIC, 0xc6)
#define DPRAM_PHONE_RAMDUMP_OFF   _IO(IOC_SEC_MAGIC, 0xc7)
#define DPRAM_GET_DGS_INFO       _IOR(IOC_SEC_MAGIC, 0xc8, unsigned char [DPRAM_DGS_INFO_BLOCK_SIZE])
#define DPRAM_NVPACKET_DATAREAD		_IOR(IOC_SEC_MAGIC, 0xc9, unsigned int)

#else
#define IOC_MZ_MAGIC				('h')
#define HN_DPRAM_PHONE_ON			_IO(IOC_MZ_MAGIC, 0xd0)
#define HN_DPRAM_PHONE_OFF			_IO(IOC_MZ_MAGIC, 0xd1)
#define HN_DPRAM_PHONE_GETSTATUS	_IOR(IOC_MZ_MAGIC, 0xd2, unsigned int)
#define HN_DPRAM_PHONE_RESET		_IO(IOC_MZ_MAGIC, 0xd3)
#define HN_DPRAM_PHONE_RAMDUMP		_IO(IOC_MZ_MAGIC, 0xd4)
#define HN_DPRAM_PHONE_BOOTTYPE		_IOW(IOC_MZ_MAGIC, 0xd5, unsigned int)
#define HN_DPRAM_MEM_RW				_IOWR(IOC_MZ_MAGIC, 0xd6, unsigned long)
#define HN_DPRAM_WAKEUP		_IOW(IOC_MZ_MAGIC, 0xd7, unsigned int)
#define HN_DPRAM_SILENT_RESET		_IO(IOC_MZ_MAGIC, 0xd8)
#endif

int main(int argc, char *argv[]) {
  if (!argv[1]) {
    printf("failed! \n");
    return -1;
  }

  int idx = strtoull(argv[1], NULL, 0);
  
  printf("index %d\n", idx);
  
  int fd = open("/dev/dpram0", O_RDWR | O_NDELAY);

  if(fd < 0) {
	  printf("Failed to open dpram tty\n");
	  return -1;
  }
  
#if 1 
  switch(idx) {
    case 0:
      ioctl(fd, DPRAM_PHONE_POWON);
      break;
      
    case 1:
      ioctl(fd, DPRAM_PHONE_OFF);
      break;	
      
    case 2:
      ioctl(fd, DPRAM_PHONE_ON);
      break;
      
    case 3:
      ioctl(fd, DPRAM_PHONE_RESET);
      break;      
  }
#else 
ioctl(fd, DPRAM_PHONE_POWON);
sleep(5);
ioctl(fd, DPRAM_PHONE_ON);
sleep(5);
ioctl(fd, DPRAM_PHONE_OFF);
sleep(5);
#endif

  close(fd);
  return 0;
}	
