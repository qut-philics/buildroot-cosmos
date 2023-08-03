#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

#define REGS_BASEADDR 	0x80000000
#define REGS_MAPSIZE (4*1024)

int main() {
	int fd;
	int runmap;
	uint32_t* pregs;

	uint32_t timestamp;

	uint32_t day;
	uint32_t month;
	uint32_t year;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;

	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd == -1) {
		printf("ERROR: Failed to open /dev/mem for mapping.\n");
		return -1;
	}

	pregs = mmap(0, REGS_MAPSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, REGS_BASEADDR);
	if  (pregs == MAP_FAILED) {
		printf("ERROR: Failed to map REGS to memory.\n");
		return -1;
	}

	timestamp = *(volatile uint32_t*)pregs;
	
	day 	= (timestamp >> 27) & (0x1F);
	month 	= (timestamp >> 23) & (0x0F);
	year 	= (timestamp >> 17) & (0x3F);
	year 	+= 2000;

	hour 	= (timestamp >> 12) & (0x1F);
	minute 	= (timestamp >> 6 ) & (0x3F);
	second 	= (timestamp      ) & (0x3F);

	printf("Bitstream timestamp is %d/%d/%d %02d:%02d:%02d [0x%08X]\r\n", day, month, year, hour, minute, second, timestamp);

	runmap = munmap(pregs, REGS_MAPSIZE);
	if  (runmap == -1) {
		printf("ERROR: Failed to unmap REGS.\n");
		return -1;
	}

	return 0;
}


