#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


#define FILE_FPGA "/sys/class/fpga_manager/fpga0/firmware"
#define FILE_FPGA_FLAGS "/sys/class/fpga_manager/fpga0/flags"
#define FILE_FPGA_STATE "/sys/class/fpga_manager/fpga0/state"

#define DIR_FIRMWARE "/lib/firmware"
#define FILE_BIN "configpl.bit.bin"
#define PATH_BIN DIR_FIRMWARE "/" FILE_BIN


int main(int argc, char *argv[])
{
    uint8_t i;
    char line[32];

	if (argc != 2) {
    	printf("Usage: configpl <bitstream.bin>\n");
    	return 0;
    }

    uint8_t din[4], dout[4];
    FILE *bs;
    FILE *bin;
    FILE *fpga;
    FILE *fpga_flags;
    FILE *fpga_state;

    printf("Opening bitstream %s...", argv[1]);
    bs = fopen(argv[1],"rb");
    if (bs == NULL) {
    	printf("FAILED (Error %d).\n", errno);
    	return -1;
    }
    printf("DONE.\n");

    printf("Checking for firmware directory (%s)...", DIR_FIRMWARE);
    if (mkdir(DIR_FIRMWARE, 0755) == -1) {
    	if (errno != EEXIST) {
    		printf("FAILED (Error %d).\n", errno);
    		return -1;
    	}
    }
    printf("DONE.\n");

    printf("Opening firmware binary for writing (%s)...", PATH_BIN);
    bin = fopen(PATH_BIN,"wb");
    if (bin == NULL) {
    	printf("FAILED (Error %d).\n", errno);
    	return -1;
    }
    printf("DONE.\n");

    printf("Writing dword-swapped firmware binary...");
    do {
    	fread(din,4,1,bs);
    	for (i = 0; i < 4; i++) {
    		dout[3-i] = din[i];
    	}
    	fwrite(dout,4,1,bin);
    } while (!feof(bs));
    fclose(bs);
    fflush(bin);
    fclose(bin);
    printf("DONE.\n");


    printf("Opening fpga manager flags (%s)...", FILE_FPGA_FLAGS);
	fpga_flags = fopen(FILE_FPGA_FLAGS,"w");
	if (fpga_flags == NULL) {
		printf("FAILED (Error %d).\n", errno);
		return -1;
	}
	printf("DONE.\n");

	printf("Setting fpga manager flags...");
	fprintf(fpga_flags, "0");
	fflush(fpga_flags);
	fclose(fpga_flags);
	printf("DONE.\n");

    printf("Opening fpga manager firmware device (%s)...", FILE_FPGA);
	fpga = fopen(FILE_FPGA,"w");
	if (fpga == NULL) {
		printf("FAILED (Error %d).\n", errno);
	}
	printf("DONE.\n");

	printf("Writing firmware name (%s) to fpga manager...", FILE_BIN);
	fprintf(fpga, FILE_BIN);
	fflush(fpga);
	fclose(fpga);
	printf("DONE.\n");


	printf("Checking fpga state...");
	sleep(1);
	fpga_state = fopen(FILE_FPGA_STATE,"r");
	if (fpga_state == NULL) {
		printf("FAILED (Error %d).\n", errno);
		return -1;
	}
	printf("DONE.\n");

	fscanf(fpga_state, "%s", line);
	printf("FPGA status = %s\n", line);
	fclose(fpga_state);

    return 0;
}
