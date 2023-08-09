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
#define SV_I_F				(*((volatile uint16_t *)(SV_BASE + 0x00)))	//TODO: These should be signed???
#define SV_I_L				(*((volatile uint16_t *)(SV_BASE + 0x02)))
#define SV_V_C				(*((volatile uint16_t *)(SV_BASE + 0x04)))
#define SV_V_R				(*((volatile uint16_t *)(SV_BASE + 0x06)))
#define SV_V_DCP			(*((volatile uint16_t *)(SV_BASE + 0x08)))
#define SV_V_DCN			(*((volatile uint16_t *)(SV_BASE + 0x08)))


#define CTRL_BASE			0xFFFFF800UL
// PWM compare
#define CTRL_CMPA			(*((volatile uint16_t *)(CTRL_BASE + 0x00)))
#define CTRL_CLKDIV			(*((volatile uint16_t *)(CTRL_BASE + 0x02)))
#define CTRL_PHASEINC		(*((volatile uint32_t *)(CTRL_BASE + 0x04)))
// Auxilliary control word (unused)
#define CTRL_AUX			(*((volatile uint16_t *)(CTRL_BASE + 0x02)))
// RTDS (unused)
#define CTRL_RTDS_UI_H		(*((volatile uint16_t *)(CTRL_BASE + 0x04)))
#define CTRL_RTDS_UI_L		(*((volatile uint16_t *)(CTRL_BASE + 0x06)))
#define CTRL_RTDS_AUX_H		(*((volatile uint16_t *)(CTRL_BASE + 0x08)))
#define CTRL_RTDS_AUX_L		(*((volatile uint16_t *)(CTRL_BASE + 0x0A)))

//Busy wait definitions
#define TIMER_500MS 0x27AE000UL

//3x blue LED flash
void signalStartup(void) {
	uint32_t count = 6;
	volatile uint32_t timer = TIMER_500MS;
	uint32_t regval = LEDS_B;

	while(count > 0) {
		if(!(--timer)) {
			REG_LEDS = regval;
			regval = regval^LEDS_B;
			timer = TIMER_500MS;
			count--;
		}
	}
}

//Programme entry
__attribute__((optimize("unroll-loops")))
int main(void) {
	//uint16_t duty16 = 0;

	signalStartup();	//3x blue flash

	//Configure PWM settings
	CTRL_CMPA = 0x8000;			// 50% duty
	CTRL_CLKDIV = 0x0000;		// Modulator clock /1 = 200 MHz
	CTRL_PHASEINC = 0x000346DC;	// 10 kHz @ 200 MHz
	REG_TRIGGERS = 1;			// Manually trigger PS-PL DMA to update settings

	//ctrllatsync must be set to non-zero value for control loop to execute
	REG_CTRLLATSYNC = 1;

	REG_TRIPMASK = ~(0ULL);		//Mask all trip sources
	REG_TRIPCTRL = (1UL<<31);	//Reset trip module

	//This should be done as last step before dropping into control loop
	REG_WDCTRL = 0xCAFEF00DUL;	//Reset watchdog
	REG_TRIPMASK = ~(1UL<<BP_TRIPMASK_WD);	//Enable watchdog trip source only
	REG_WDCTRL = 0xFEEDBEEFUL;	//Service watchdog

	while(1) {
		//Wait for trigger
		asm("dsb\nwfi");


		//CRITICAL CODE GOES HERE;
		
		// For closed loop control, here's where we could read sampled
		// values and calculate an updated control action (PWM duty) 
		
		//duty16 = SV_V_C;
		//CTRL_CMPA = duty16;

		//Signal critical code complete (init's DMA to PL)
		asm("dsb\nsev");
		
		

		//NON-CRITICAL CODE GOES HERE
		
		// Any possible precalculation or housekeeping should be done 
		// here to minimise control action latency. Code here still
		// needs to complete within control interval to avoid control
		// timestep overflow.

		//Flash green LED as timestamp increases
		REG_LEDS = (SV_TIMEL>>24)&LEDS_G;
		//Service watchdog
		REG_WDCTRL = 0xFEEDBEEFUL;		
		
	}

}
