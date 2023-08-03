#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "si5338-registers.h"
#define ADDR_CLKGEN 	0x70

#define DEV_I2C_MUX0 "/dev/i2c-1"
#define DEV_I2C_MUX1 "/dev/i2c-2"

//Devices on I2C
#define ADDR_SYSCTRL	0x0D
//Devices on I2C_MUX0
#define ADDR_CLKGEN 	0x70
#define ADDR_MGTMUX0 	0x30
#define ADDR_MGTMUX1 	0x32
#define ADDR_MGTMUX2 	0x34
#define ADDR_MGTMUX3 	0x36
#define ADDR_GPIOSFP	0x20
//Devices on I2C_MUX1
#define ADDR_MGTMUX4 	0x30
#define ADDR_MGTMUX5 	0x32
#define ADDR_MGTMUX6 	0x34
#define ADDR_MGTMUX7 	0x36

int i2c_reg_write(int fd, uint8_t address, uint8_t data) {
	int result;	
	uint8_t buf[2];

	buf[0] = address;
	buf[1] = data;
	
	result = write(fd, buf, 2);
	if (result < 2) {
		printf("ERROR: I2C write failed with error code %d\r\n", result);
		return -1;
	}
	
	//printf("Wrote 0x%02X to register 0x%02X.\r\n", data, address);	
	return 0;
} 

int i2c_reg_read(int fd, uint8_t address, uint8_t* data) {
	int result;	
	uint8_t buf;

	buf = address;
	
	result = write(fd, &buf, 1);
	if (result < 1) {
		printf("ERROR: I2C write failed with error code %d\r\n", result);
		return -1;
	}

	result = read(fd, &buf, 1);
	if (result < 1) {
		printf("ERROR: I2C read failed with error code %d\r\n", result);
		return -1;
	}

	*data = buf;

	//printf("Read 0x%02X from register 0x%02X.\r\n", *data, address);
	return 0;
}

int i2c_reg_rmw(int fd, uint8_t address, uint8_t data, uint8_t mask) {
	uint8_t reg;
	int result;

	result = i2c_reg_read(fd, address, &reg); 
	if (result < 0) {
		printf("ERROR: Failed to get register value at address 0x%02X\r\n", address);
		return -1;
	}

	reg |= (data & mask);
	reg &= (data | ~mask);

	i2c_reg_write(fd, address, reg);
	if (result < 0) {
		printf("ERROR: Failed to set register value at address 0x%02X\r\n", address);
		return -1;
	}
}

int main() {
	int fd;
	int result;
	int i;
	uint8_t mask;
	uint8_t reg;
	
	printf("cosmos-i2cconfig: Opening %s for configuration...", DEV_I2C_MUX0);
	//Open I2C device for writing on bus I2C_MUX0
	fd = open(DEV_I2C_MUX0, O_RDWR);
	if (fd < 0) {
		printf("ERROR: Failed to open I2C device %s\r\n", DEV_I2C_MUX0);
		return fd;	
	}
	printf("DONE.\r\n");

	printf("cosmos-i2cconfig: Configuring Si5338 clock generator...");
	//Set slave address for clock generator
	result = ioctl(fd, I2C_SLAVE, ADDR_CLKGEN);
	if (result < 0) {
		printf("ERROR: Failed to set slave address to 0x%02X\r\n", ADDR_CLKGEN);
		return result;	
	}
	
	
	//Configure Si5338 clock generator
	i2c_reg_rmw(fd, 230, 0x10, 0x10);	// SBR, bit 4
 	i2c_reg_rmw(fd, 241, 0x80, 0x80);	// SBR, bit 7
	//Dump registers
	for (i = 0; i < NUM_REGS_MAX; i++) {
		mask = Reg_Store[i].Reg_Mask;
		if (mask == 0) {
			continue;	//Read only
		} else if (mask == 0xFF) {
			i2c_reg_write(fd, Reg_Store[i].Reg_Addr, Reg_Store[i].Reg_Val);		
		} else {
			i2c_reg_rmw(fd, Reg_Store[i].Reg_Addr, Reg_Store[i].Reg_Val, Reg_Store[i].Reg_Mask);		
		}
	}
	do {
		i2c_reg_read(fd, 218, &reg);
	} while (reg & 0x04);
	i2c_reg_rmw(fd, 49, 0x7F, 0x80);	// CBR, bit 7
	i2c_reg_rmw(fd, 246, 0x02, 0x02);	// SBR, bit 1
	usleep(25000);				// Wait 25ms
	i2c_reg_rmw(fd, 241, 0x7F, 0x80);	// CBR, bit 7
	i2c_reg_write(fd, 241, 0x65);
	do {
		i2c_reg_read(fd, 218, &reg);
	} while (reg & 0x15);
	i2c_reg_read(fd, 237, &reg);		// Copy 237[1:0] to 47[1:0]
	i2c_reg_rmw(fd, 47, reg, 0x03);
	i2c_reg_read(fd, 236, &reg);		// Copy 236 to 46
	i2c_reg_write(fd, 46, reg);	
	i2c_reg_read(fd, 235, &reg);		// Copy 235 to 45
	i2c_reg_write(fd, 45, reg);
	i2c_reg_rmw(fd, 49, 0x80, 0x80);	// SBR, bit 7
	i2c_reg_rmw(fd, 230, 0xEF, 0x10);	// CBR, bit 4
	printf("DONE.\r\n");
	
	printf("cosmos-i2cconfig: Routing clock generator to GTX clock net...");
	//Set slave address for MGT Mux 2
	result = ioctl(fd, I2C_SLAVE, ADDR_MGTMUX2);
	if (result < 0) {
		printf("ERROR: Failed to set slave address to 0x%02X\r\n", ADDR_MGTMUX2);
		return result;	
	}
	// Route clkgen0 to gtx
	i2c_reg_write(fd, 0x01, 0x80);
	i2c_reg_write(fd, 0x06, 0x40);
	i2c_reg_write(fd, 0x08, 0x10);
	printf("DONE.\r\n");
	
	printf("cosmos-i2cconfig: Enabling SFP tranceivers...");
	//Set slave address for SFP GPIO expander
	result = ioctl(fd, I2C_SLAVE, ADDR_GPIOSFP);
	if (result < 0) {
		printf("ERROR: Failed to set slave address to 0x%02X\r\n", ADDR_GPIOSFP);
		return result;	
	}
	// Enable all SFP transceivers
	i2c_reg_write(fd, 0x46, 0x2D);
	i2c_reg_write(fd, 0x47, 0x2D);
	i2c_reg_write(fd, 0x02, 0xED);
	i2c_reg_write(fd, 0x03, 0xED);
	i2c_reg_write(fd, 0x06, 0xED);
	i2c_reg_write(fd, 0x07, 0xED);
	printf("DONE.\r\n");

	printf("cosmos-i2cconfig: Enabling LEDs and DIP switches...");
	//Set slave address for SFP GPIO expander
	result = ioctl(fd, I2C_SLAVE, ADDR_SYSCTRL);
	if (result < 0) {
		printf("ERROR: Failed to set slave address to 0x%02X\r\n", ADDR_SYSCTRL);
		return result;	
	}
	// Select LEDs and User DIP switches
	i2c_reg_write(fd, 0x08, 0xFE);
	printf("DONE.\r\n");

	result = close(fd);
	if (result < 0) {
		printf("ERROR: Failed to close I2C device\r\n");
		return result;	
	}

	return 0;
}


