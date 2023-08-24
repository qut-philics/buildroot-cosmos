#include <stdint.h>
#include "smartdac_regs.h"

//PL register definitions
#define REGS_BASE 			SMARTDAC_DEFAULT_BASEADDR

//LEDs
#define REG_LEDS_OFFSET 	LEDCTRL_OFFSET
#define REG_LEDS  			*((volatile uint32_t *)(REGS_BASE + REG_LEDS_OFFSET))

//Watchdog
#define REG_WDCTRL 			*((volatile uint32_t *)(REGS_BASE + WDCTRL_OFFSET))
	// Service: 	0xFEEDBEEF
	// Reset: 		0xCAFEF00D

//TRIP MODULE
#define REG_TRIPCTRL 		*((volatile uint32_t *)(REGS_BASE + TRIPCTRL_OFFSET))
	// Bit 31 - Trip clear
#define REG_TRIPMASK 		*((volatile uint64_t *)(REGS_BASE + TRIPMASK_OFFSET))
	// Bit assignment as below. Set bit high to disable trip channel.
#define REG_TRIPSTS 		*((volatile uint64_t *)(REGS_BASE + TRIPSTS_OFFSET))
#define REG_TRIPSRC 		*((volatile uint64_t *)(REGS_BASE + TRIPSRC_OFFSET))
	// Bit 0 - I_F		(check ADC order??)
	// Bit 1 - I_L
	// Bit 2 - V_C
	// Bit 3 - V_R
	// Bit 4 - V_DCP
	// Bit 5 - V_DCN
	// Bit 6 - Watchdog
	// Bit 7 - eStop
	// Bit 8 - G3TGDI (A67)
	// Bit 9 - RTDS overflow

#define BP_TRIPMASK_WD 6 //Define bit-position for WD in trip mask

//CONTROL INTERVAL/SYNC
#define REG_CTRLLATSYNC 		*((volatile uint32_t *)(REGS_BASE + CTRLLATSYNC_OFFSET))

//TRIGGERS
#define REG_TRIGGERS 		*((volatile uint32_t *)(REGS_BASE + TRIGGERS_OFFSET))

#define LEDS_R 				0x00000001UL
#define LEDS_G 				0x00000002UL
#define LEDS_B 				0x00000004UL


#define SV_BASE				0xFFFFF000UL
// Timestamp (64-bit)
#define SV_TIMEL			(*((volatile uint32_t *)(SV_BASE + 0x00)))
#define SV_TIMEH			(*((volatile uint32_t *)(SV_BASE + 0x04)))
// ADC samples
#define SV_I_F				(*((volatile int16_t *)(SV_BASE + 0x08)))
#define SV_I_L				(*((volatile int16_t *)(SV_BASE + 0x0A)))
#define SV_V_C				(*((volatile int16_t *)(SV_BASE + 0x0C)))
#define SV_V_R				(*((volatile int16_t *)(SV_BASE + 0x0E)))
#define SV_V_DCP			(*((volatile int16_t *)(SV_BASE + 0x10)))
#define SV_V_DCN			(*((volatile int16_t *)(SV_BASE + 0x12)))
#define SV_RTDS_VCP			(*((volatile int32_t *)(SV_BASE + 0x14)))
#define SV_RTDS_VSRC		(*((volatile int32_t *)(SV_BASE + 0x18)))

#define CTRL_BASE			0xFFFFF800UL
// RTDS
#define CTRL_RTDS_UI		(*((volatile int32_t *)(CTRL_BASE + 0x00)))
#define CTRL_RTDS_AUX		(*((volatile int32_t *)(CTRL_BASE + 0x04)))
// PWM compare
#define CTRL_CMPA			(*((volatile uint16_t *)(CTRL_BASE + 0x08)))

//3x blue LED flash
void signalStartup(void) {
	
	// LEDs off
	REG_LEDS = 0;
	
	// Flash blue LED 3x
	while (SV_TIMEL & (1 << 25)); REG_LEDS = LEDS_B;
	while (~SV_TIMEL & (1 << 25)); REG_LEDS = 0;
	while (SV_TIMEL & (1 << 25)); REG_LEDS = LEDS_B;
	while (~SV_TIMEL & (1 << 25)); REG_LEDS = 0;
	while (SV_TIMEL & (1 << 25)); REG_LEDS = LEDS_B;
	while (~SV_TIMEL & (1 << 25)); REG_LEDS = 0;
	
}

//Programme entry
__attribute__((optimize("unroll-loops")))
int main(void) {

	signalStartup();	//3x blue flash

	//Configure PWM settings
	CTRL_CMPA = 0x8000;			// 50% duty

	//ctrllatsync must be set to non-zero value for control loop to execute
	REG_CTRLLATSYNC = 1;

	REG_TRIPMASK = ~(0ULL);		//Mask all trip sources
	REG_TRIPCTRL = (1UL<<31);	//Reset trip module

	//This should be done as last step before dropping into control loop
	REG_WDCTRL = 0xCAFEF00DUL;				//Reset watchdog
	REG_TRIPMASK = ~(1UL<<BP_TRIPMASK_WD);	//Enable watchdog trip source only
	REG_WDCTRL = 0xFEEDBEEFUL;				//Service watchdog

	while(1) {
		//Wait for trigger
		asm("dsb\nwfi");

		//CRITICAL CODE GOES HERE;
		
		// Simulate a perfect 2R resistor (Icp = Vcp/2)
		CTRL_RTDS_UI = SV_RTDS_VCP >> 1;
		
		// Send lower 32-bits of timestamp to RTDS 
		// This should be plottable in the RTDS as a ramp
		CTRL_RTDS_AUX = SV_TIMEL;

		//Signal critical code complete (init's DMA to PL)
		asm("dsb\nsev");
		
		//NON-CRITICAL CODE GOES HERE

		//Flash green LED as timestamp increases
		REG_LEDS = (SV_TIMEL>>24)&LEDS_G;
		//Service watchdog
		REG_WDCTRL = 0xFEEDBEEFUL;		
		
	}

}
