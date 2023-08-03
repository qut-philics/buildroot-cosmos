#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include "elf.h"

// Size of each input chunk to be read and allocate for.
#define  CHUNK  262144

//Allowable memory regions
#define MEM_DDR_START	0x3E000000ULL
#define MEM_DDR_LEN		0x00400000ULL

#define MEM_OCM_START	0xFFFE0000ULL
#define MEM_OCM_LEN		0x00020000ULL

//Registers
#define REG_SLCR_BASE			0xF8000000U
#define SLCR_A9_CPU_RST_CTRL	0x244
#define A9_CPU_RST				0x01
#define A9_CPU_CLKSTOP			0x10
#define PAGE_SIZE				0x1000

//Trampoline code
#define TRAMPOLINE0		0xE51F0000ULL		// ldr r0, [pc, #-0]
#define TRAMPOLINE1		0xE12FFF10ULL		// bx r0
#define TRAMPOLINE2		0x00000000			// .word 0	(reserved for entry address)

int fd;

void printUsage() {
	printf("USAGE: zrproc <start|stop|status> [firmware.elf]\n");
}

int loadELF(char* elf, uint32_t* entry) {
	Elf32_Ehdr* ehdr;
	Elf32_Phdr* phdr;
	
	uint32_t phoff;			//Program header table offset
	uint16_t phentsize;		//Program header table entry size
	uint16_t phnum;			//Number of program header table entries 
	
	//uint32_t shoff;			//Section header table offset
	//uint16_t shentsize;		//Section header table entry size
	//uint16_t shnum;			//Number of section header table entries
	//uint16_t shstrndx;		//Index of entry that contains section names
	
	int i, pass, globalpass;
	
	uint64_t ddrUsed = 0;
	uint64_t ocmUsed = 0;
	
	uint8_t *srcBase;
	uint32_t srcOffset;
	uint8_t *dstBase;
	uint32_t dstOffset;
	uint32_t copyLength;
	
	uint8_t* pDDR;
	uint8_t* pOCM;
	
	//Use struct to read ELF header
	ehdr = (Elf32_Ehdr*) elf;
	
	printf("\nChecking firmware file compatibillity:\n");
	
	printf("  ELF magic number:              ");
	if (strncmp(ehdr->e_ident, ELFMAG, 4)) {
		printf("FAILED (0x%02X%02X%02X%02X)\n", ehdr->e_ident[EI_MAG0], ehdr->e_ident[EI_MAG1], ehdr->e_ident[EI_MAG2], ehdr->e_ident[EI_MAG3]);
		return 1;
	} else {
		printf("OK\n");
	}
	
	printf("  ELF class (32-bit):            ");
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
		printf("FAILED (0x%02X)\n", ehdr->e_ident[EI_CLASS]);
		return 1;
	} else {
		printf("OK\n");
	}
	
	printf("  ELF specification version:     ");
	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
		printf("FAILED (0x%02X)\n", ehdr->e_ident[EI_VERSION]);
		return 1;
	} else {
		printf("OK\n");
	}
	
	printf("  Data encoding (little endian): ");
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		printf("FAILED (0x%02X)\n", ehdr->e_ident[EI_DATA]);
		return 1;
	} else {
		printf("OK\n");
	}
	
	printf("  Target OS ABI (none):          ");
	if (ehdr->e_ident[EI_OSABI] != ELFOSABI_NONE) {
		printf("FAILED (0x%02X)\n", ehdr->e_ident[EI_OSABI]);
		return 1;
	} else {
		printf("OK\n");
	}
	
	printf("  Object file type (executable): ");
	if (ehdr->e_type != ET_EXEC) {
		printf("FAILED (0x%04X)\n", ehdr->e_type);
		return 1;
	} else {
		printf("OK\n");
	}
	
	printf("  Target architecture (ARM):     ");
	if (ehdr->e_machine != EM_ARM) {
		printf("FAILED (0x%04X)\n", ehdr->e_machine);
		return 1;
	} else {
		printf("OK\n");
	}
	
	printf("  File version (current):        ");
	if (ehdr->e_version != EV_CURRENT) {
		printf("FAILED (0x%08X)\n", ehdr->e_version);
		return 1;
	} else {
		printf("OK\n");
	}
	
	//Processor specific flags
	printf("\nProcessor flags:\n");
	
	printf("  * ARM EABI version %d\n", EF_ARM_EABI_VERSION(ehdr->e_flags)>>24);
	
	if ((ehdr->e_flags) & EF_ARM_ABI_FLOAT_HARD) {
		printf("  * Hardware floating point\n");
	}
	
	if ((ehdr->e_flags) & EF_ARM_ABI_FLOAT_SOFT) {
		printf("  * Software floating point\n");
	}
	
	//Get ELF file strucure from header
	phoff = ehdr->e_phoff;			//Program header table offset
	phentsize = ehdr->e_phentsize;	//Program header table entry size
	phnum = ehdr->e_phnum;			//Number of program header table entries 
	
	//shoff = ehdr->e_shoff;			//Section header table offset
	//shentsize = ehdr->e_shentsize;	//Section header table entry size
	//shnum = ehdr->e_shnum;			//Number of section header table entries
	//shstrndx = ehdr->e_shstrndx;		//Index of entry that contains section names
	
	*entry = ehdr->e_entry;			//Program entry point address
	
	printf("\nChecking loadable sections:\n");
	
	printf("\n  Idx  Memory Address  Size     LoadSize OK?\n");
	printf("  --------------------------------------------------\n");
	
	globalpass = 1;
	for (i = 0; i < phnum; i++) {
		phdr = (Elf32_Phdr*) (elf+phoff+(i*phentsize));
		pass = 1;
		if (phdr->p_type == PT_LOAD) {
			printf("  0x%02X ",i);
			if ((phdr->p_paddr >= MEM_DDR_START) && (phdr->p_paddr < MEM_DDR_START+MEM_DDR_LEN)) {
				printf("DDR    ");
				ddrUsed += phdr->p_memsz;
				if (phdr->p_paddr+phdr->p_memsz > MEM_DDR_START+MEM_DDR_LEN) {
					pass = 0;
				}
			} else if ((phdr->p_paddr >= MEM_OCM_START) && (phdr->p_paddr < MEM_OCM_START+MEM_OCM_LEN)) {
				printf("OCM    ");
				ocmUsed += phdr->p_memsz;
				if (phdr->p_paddr+phdr->p_memsz > MEM_OCM_START+MEM_OCM_LEN) {
					pass = 0;
				}
			} else {
				printf("???    ");
				pass = 0;
			}
			printf("%08X ",phdr->p_paddr);
			printf("%08X ",phdr->p_memsz);
			printf("%08X ",phdr->p_filesz);
			if (pass) {
				printf("OK\n");
			} else {
				printf("FAIL\n");
			}
		}
		if (!pass) globalpass = 0;
	}
	printf("\n");
	printf("  Total DDR utilisation: % 8lld of % 8lld bytes (% 5.1f%%). ", ddrUsed, MEM_DDR_LEN, 100.0*ddrUsed/MEM_DDR_LEN);
	if (ddrUsed < MEM_DDR_LEN) {
		printf("OK\n"); 
	} else {
		printf("FAIL\n");
		globalpass = 0;
	}
	printf("  Total OCM utilisation: % 8lld of % 8lld bytes (% 5.1f%%). ", ocmUsed, MEM_OCM_LEN, 100.0*ocmUsed/MEM_OCM_LEN);
	if (ocmUsed < MEM_OCM_LEN) {
		printf("OK\n"); 
	} else {
		printf("FAILED!\n");
		globalpass = 0;
	}
	printf("\n");
	
	if (!globalpass) {
		printf("ERROR: Firmware image load destination is invlaid.\n");
		return 1;
	}
	
	printf("Firmware image verified.\n\n");
	
	printf("Loading firmware image to target memory regions:\n\n");
	
	printf("  Mapping DDR at %08llX-%08llX...",MEM_DDR_START,MEM_DDR_START+MEM_DDR_LEN-1); 
	//pDDR = malloc(MEM_DDR_LEN); //TODO: Replace with mmap
	pDDR = mmap(NULL, MEM_DDR_LEN, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, MEM_DDR_START);
	if (pDDR == MAP_FAILED) {
		printf("FAILED.\nERROR: Failed to map DDR.\n");
		return 1;
	}
	printf("DONE\n");
	
	printf("  Mapping OCM at %08llX-%08llX...",MEM_OCM_START,MEM_OCM_START+MEM_OCM_LEN-1);
	//pOCM = malloc(MEM_OCM_LEN); //TODO: Replace with mmap
	pOCM = mmap(NULL, MEM_OCM_LEN, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, MEM_OCM_START);
	if (pOCM == MAP_FAILED) {
		printf("FAILED.\nERROR: Failed to map OCM.\n");
		return 1;
	}
	printf("DONE\n");
	
	
	printf("\n  Copying loadable sections to target memory...\n");
	srcBase = elf;
	for (i = 0; i < phnum; i++) {
		phdr = (Elf32_Phdr*) (elf+phoff+(i*phentsize));
		if (phdr->p_type == PT_LOAD) {
			srcOffset = phdr->p_offset;
			copyLength = phdr->p_filesz;
			if (copyLength == 0) continue;
			printf("  [0x%02X] OFFSET %08X, LEN %08X ==> ",i,srcOffset,copyLength);
			
			if ((phdr->p_paddr >= MEM_DDR_START) && (phdr->p_paddr < MEM_DDR_START+MEM_DDR_LEN)) {
				dstBase = pDDR;
				dstOffset = phdr->p_paddr-MEM_DDR_START;
				printf("DDR %08llX-%08llX\n",MEM_DDR_START+dstOffset,MEM_DDR_START+dstOffset+copyLength-1);
			} else if ((phdr->p_paddr >= MEM_OCM_START) && (phdr->p_paddr < MEM_OCM_START+MEM_OCM_LEN)) {
				dstBase = pOCM;
				dstOffset = phdr->p_paddr-MEM_OCM_START;
				printf("OCM %08llX-%08llX\n",MEM_OCM_START+dstOffset,MEM_OCM_START+dstOffset+copyLength-1);
			}
			
			memcpy(dstBase+dstOffset, srcBase+srcOffset, copyLength);
			
		}
	}
	printf("\nSuccessfully loaded application to target memory regions.\n");
	
	//TODO: Rempace with munmap
	//free(pDDR);
	//free(pOCM);
	__clear_cache(pDDR, pDDR+MEM_DDR_LEN);
	__clear_cache(pOCM, pOCM+MEM_OCM_LEN);

	munmap(pDDR, MEM_DDR_LEN);
	munmap(pOCM, MEM_OCM_LEN);
	
	return 0;
}

int loadFirmware(const char* filename, uint32_t* entry) {
	FILE *fw;
	char *data = NULL;
	char *temp;
    size_t used = 0;
    size_t bytesRead;
	
	//Open firmware file
	fw = fopen(filename, "rb");
    if (ferror(fw) || (fw == NULL)) {
		printf("ERROR: Failed to open firmware file %s.\n", filename);
		return 1;
	}
    
	//Allocate memory buffer to hold firmware file
	data = malloc(CHUNK);
	if (data == NULL) {
		printf("ERROR: Couldn't allocate temporary memory for firmware file.\n");
		return 1;
	}
	
	//Read firmware into memory
	while (1) {
		//Read chunk of file
		bytesRead = fread(data + used, 1, CHUNK, fw);
		
		//Have we reached EoF?
		if (bytesRead == 0) break;
		
		//Keep count of firmware file size
		used += bytesRead;
		
		//Allocate new chunk of memory
		temp = realloc(data, used + CHUNK);
		if (temp == NULL) {
			free(data);
			printf("ERROR: Couldn't allocate temporary memory for firmware file.\n");
			return 1;
		}
		data = temp;
	}
	
	//We have a copy in memory, close firmware file
	fclose(fw);

	//Check for read errors
    if (ferror(fw)) {
        free(data);
		printf("ERROR: Error reading firmware file %s.\n", filename);
        return 1;
    }

	//Resize memory buffer to file size
    temp = realloc(data, used);
    if (temp == NULL) {
        free(data);
		printf("ERROR: Couldn't allocate temporary memory for firmware file.\n");
        return 1;
    }
    data = temp;
	
	printf("Loaded firmware file %s. Total size %d bytes.\n",filename,used);
	
	if (loadELF(data, entry)) {
		printf("ERROR: Failed to process firmware file.\n");
	}
	
	//Free allocated memory
	free(data);

    return 0;
}

int loadTrampoline(uint32_t entry) {
	uint32_t* zero;
	
	zero = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, 0x00000000);
	if (zero == MAP_FAILED) {
		printf("ERROR: Failed to map BOOT memory at 0x00000000.\n");
		return 1;
	}
	
	zero[0] = TRAMPOLINE0;
	zero[1] = TRAMPOLINE1;
	zero[2] = entry;
	
	__clear_cache(zero, zero+PAGE_SIZE);

	munmap(zero, PAGE_SIZE);

	return 0;
}

int stopCPU(int cpu) {
	uint8_t* slcr;
	uint32_t temp;
	
	slcr = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, REG_SLCR_BASE);
	if (slcr == MAP_FAILED) {
		printf("ERROR: Failed to map SLCR registers at 0x%08lX.\n", REG_SLCR_BASE);
		return 1;
	}
	
	//temp = *(slcr+SLCR_A9_CPU_RST_CTRL);
	//temp |= (A9_CPU_RST<<cpu)|(A9_CPU_CLKSTOP<<cpu);
	//*(slcr+SLCR_A9_CPU_RST_CTRL) = temp;

	//*(volatile uint32_t *)(slcr+SLCR_A9_CPU_RST_CTRL) = (A9_CPU_RST<<cpu)|(A9_CPU_CLKSTOP<<cpu);
	*(volatile uint32_t *)(slcr+SLCR_A9_CPU_RST_CTRL) = (A9_CPU_RST<<cpu);
	*(volatile uint32_t *)(slcr+SLCR_A9_CPU_RST_CTRL) = (A9_CPU_RST<<cpu)|(A9_CPU_CLKSTOP<<cpu);

	__clear_cache(slcr, slcr+PAGE_SIZE);

	munmap(slcr, PAGE_SIZE);
	
	return 0;
}

int startCPU(int cpu) {
	uint8_t* slcr;
	uint32_t temp;
	
	slcr = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, REG_SLCR_BASE);
	if (slcr == MAP_FAILED) {
		printf("ERROR: Failed to map SLCR registers at 0x%08lX.\n", REG_SLCR_BASE);
		return 1;
	}
	
	//temp = *(slcr+SLCR_A9_CPU_RST_CTRL);
	//temp &= ~((A9_CPU_RST<<cpu)|(A9_CPU_CLKSTOP<<cpu));
	//*(slcr+SLCR_A9_CPU_RST_CTRL) = temp;
	//*(volatile uint32_t *)(slcr+SLCR_A9_CPU_RST_CTRL) = 0x00000000;
	*(volatile uint32_t *)(slcr+SLCR_A9_CPU_RST_CTRL) = (A9_CPU_CLKSTOP<<cpu);
	*(volatile uint32_t *)(slcr+SLCR_A9_CPU_RST_CTRL) = 0x00000000;

	__clear_cache(slcr, slcr+PAGE_SIZE);

	munmap(slcr, PAGE_SIZE);
	
	return 0;
}

int statusCPU(int cpu, uint32_t* status) {
	uint8_t* slcr;
	uint32_t temp;
	
	slcr = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, fd, REG_SLCR_BASE);
	if (slcr == MAP_FAILED) {
		printf("ERROR: Failed to map SLCR registers at 0x%08lX.\n", REG_SLCR_BASE);
		return 1;
	}
	
	temp = *(volatile uint32_t *)(slcr+SLCR_A9_CPU_RST_CTRL);
	temp &= (A9_CPU_RST<<cpu)|(A9_CPU_CLKSTOP<<cpu);
	*status = (temp>>cpu);

	__clear_cache(slcr, slcr+PAGE_SIZE);

	munmap(slcr, PAGE_SIZE);
	
	return 0;
}

int openDevMem() {
	fd = open ("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 1) {
        printf("ERROR: Unable to open /dev/mem with read/write permissions.\n");
		return 1;
    }
	return 0;
}

void closeDevMem() {
	close(fd);
}

int main(int argc, char* argv[]) {
	uint32_t status;
	uint32_t entry;
	
	printf("=======================================================================\n");
	printf("==================== Zynq-7000 RemoteProc Utillity ====================\n");
	printf("=======================================================================\n\n");
	
	if (argc < 2) {
		printUsage();
		return 0;
	}
	
	if (strcmp(argv[1], "status") == 0) {
		if (openDevMem()) {
			return 0;
		}
		if (statusCPU(1, &status)) {
			printf("ERROR: Failed to get CPU1 status.\n");
			closeDevMem();
			return 0;
		}
		switch (status) {
			case 0:
				printf("STATUS: CPU1 is 'up'.\n");
				break;
			case (A9_CPU_RST|A9_CPU_CLKSTOP):
				printf("STATUS: CPU1 is 'down'.\n");
				break;
			case A9_CPU_RST:
				printf("STATUS: CPU1 is in 'reset'.\n");
				break;
			case A9_CPU_CLKSTOP:
				printf("STATUS: CPU1 is in 'clock stop'.\n");
				break;
			default:
				printf("ERROR: CPU1 status is unknown.\n");
				break;
		}
		closeDevMem();
	} else if (strcmp(argv[1], "stop") == 0) {
		printf("Stopping CPU1...");
		if (openDevMem()) {
			return 0;
		}
		if (stopCPU(1)) {
			printf("FAILED\nERROR: Failed to stop CPU1.\n");
		}
		closeDevMem();
		printf("DONE\n");
	} else if (strcmp(argv[1], "start") == 0) {
		if (argc != 3) {
			printUsage();
			return 0;
		}
		if (openDevMem()) {
			return 0;
		}
		if (statusCPU(1, &status)) {
			printf("ERROR: Failed to get CPU1 status.\n");
			closeDevMem();
			return 0;
		}
		if (status != (A9_CPU_RST|A9_CPU_CLKSTOP)) {
			printf("ERROR: CPU1 is not stopped. Use 'zrproc stop' to bring CPU1 down.\n");
			closeDevMem();
			return 0;
		}
		if (loadFirmware(argv[2], &entry)) {
			printf("ERROR: Failed to load firmware.\n");
			closeDevMem();
			return 0;
		}
		if (loadTrampoline(entry)) {
			printf("ERROR: Failed to load trampoline code.\n");
			closeDevMem();
			return 0;
		}
		if (startCPU(1)) {
			printf("ERROR: Failed to start CPU1.\n");
		}
		closeDevMem();
	} else {
		printUsage();
	}
	
	printf("\n=======================================================================\n\n");
	
	return 0;
}
