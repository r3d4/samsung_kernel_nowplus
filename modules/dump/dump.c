/*
 * Licensed under the GPL v2 or later, see the file LICENSE in this tarball.
 *  Copyright (C) 2000, Jan-Derk Bakker (J.D.Bakker@its.tudelft.nl)
 *  Copyright (C) 2008, BusyBox Team. -solar 4/26/08
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

int dumpmem(char* name, off_t start, off_t end) {
	void *map_base, *virt_addr;
	uint64_t read_result;
	uint64_t writeval = writeval; /* for compiler */
	off_t target;
	unsigned page_size, mapped_size, offset_in_page;
	int fd;
	unsigned width = 8 * sizeof(int);
	FILE* p = NULL; 

	printf("start dump mem\n");
	
	fd = open("/dev/mem", (O_RDONLY | O_SYNC));
	p = fopen(name,"wb");
		
	for ( target = start; target<=end; target+=4)
	{
		mapped_size = page_size = getpagesize();
		offset_in_page = (unsigned)target & (page_size - 1);
		if (offset_in_page + width > page_size) {
			/* This access spans pages.
			* Must map two pages to make it possible: */
			mapped_size *= 2;
		}
		map_base = mmap(NULL,
				mapped_size,
				PROT_READ,
				MAP_SHARED,
				fd,
				target& ~(off_t)(page_size - 1));
		if (map_base == MAP_FAILED)
		{
		    printf("memmap failed\n");
		    return 0;
		}

		if(p)
		{
		  printf("offset_in_page %x \n",offset_in_page);
		fwrite((char*)map_base + offset_in_page, 4,1,p);
		}
		else
		{
		  printf("open file failed\n");
		}

		if (munmap(map_base, mapped_size) == -1)
		{
		  printf("munmap failed\n");
		  return 0;
		}
	}
				
		close(fd);
		fclose(p);
}

int main(int argc, char **argv)
{
	dumpmem("./dump2030_2264.raw",0x48002030, 0x48002264);
	dumpmem("./dump25D8_25F8.raw",0x480025D8, 0x480025F8);
	dumpmem("./dump2A00_2A24.raw",0x48002A00, 0x48002A24);
	return 0;
}
