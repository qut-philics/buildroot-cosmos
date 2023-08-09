#include <stdint.h>
#include "smartdac_regs.h"

//PL register definitions
#define REGS_BASE 			SMARTDAC_DEFAULT_BASEADDR

#define REG_LEDS_OFFSET 	LEDCTRL_OFFSET
#define REG_LEDS  			*((volatile uint32_t *)(REGS_BASE + REG_LEDS_OFFSET))

#define REG_WDCTRL 			*((volatile uint32_t *)(REGS_BASE + WDCTRL_OFFSET))

#define LEDS_R 				0x00000001
#define LEDS_G 				0x00000002
#define LEDS_B 				0x00000004

#define SV_BASE				0xFFFFF000
#define SV_TEST1			*((volatile uint16_t *)(SV_BASE + 0x30))
#define SV_TEST2			*((volatile uint16_t *)(SV_BASE + 0x32))
#define SV_TEST3			*((volatile uint16_t *)(SV_BASE + 0x34))

#define SV_TIME0			*((volatile uint32_t *)(SV_BASE + 0x38))

#define CTRL_BASE			0xFFFFF800
#define CTRL_TEST1			*((volatile uint16_t *)(CTRL_BASE + 0x00))
#define CTRL_TEST2			*((volatile uint16_t *)(CTRL_BASE + 0x02))
#define CTRL_TEST3			*((volatile uint16_t *)(CTRL_BASE + 0x04))
#define CTRL_TEST4			*((volatile uint16_t *)(CTRL_BASE + 0x06))
#define CTRL_TEST5			*((volatile uint16_t *)(CTRL_BASE + 0x08))
#define CTRL_TEST6			*((volatile uint16_t *)(CTRL_BASE + 0x0A))
#define CTRL_TEST7			*((volatile uint16_t *)(CTRL_BASE + 0x0C))
#define CTRL_TEST8			*((volatile uint16_t *)(CTRL_BASE + 0x0E))

//Busy wait definitions
#define TIMER_500MS 0x27AE000UL

//3x blue LED flash
void signalStartup(void) {
	uint32_t count = 6;
	uint32_t timer = TIMER_500MS;
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
	uint32_t i = 0;
	float temp = 0.0;

	signalStartup();	//3x blue flash

	while(1) {
		//Wait for trigger
		asm("dsb\nwfi");

		//CRITICAL CODE GOES HERE;
		temp = SV_TIME0;


		for (i = 0; i < 100; i++) {
			temp += temp*temp;
		}

		CTRL_TEST1 = temp;


		//Signal critical code complete
		asm("dsb\nsev");

		//NON-CRITICAL CODE GOES HERE
		REG_LEDS = (SV_TIME0>>16)&LEDS_G;
		//REG_WDCTRL = temp;
	}

}
