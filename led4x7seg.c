/*
 * Demonstration of using a 4-character 7-segment LED display and multiple
 * timers on the MSP430G2553
 */

#include <msp430.h>
#include <stdint.h>

#define DLY_1S		4096 - 1
#define DLY_512TH	7
#define DLY_1024TH	3


/*
 * Bit  7  6  5  4  3  2  1  0
 * Seg  DP A  B  C  D  E  F  G
 *
 *  AAA
 * F   B
 *  GGG
 * E   C
 *  DDD  DP
 */
const uint8_t segNums[] = {
	0b01111110, // = 0
	0b00110000, // = 1
	0b01101101, // = 2
	0b01111001, // = 3
	0b00110011, // = 4
	0b01011011, // = 5
	0b01011111, // = 6
	0b01110000, // = 7
	0b01111111, // = 8
	0b01110011  // = 9
};

const uint8_t segLets[7] = {
	0b00001101, // = c
	0b00111101, // = d
	0b00010111, // = h
	0b01011011, // = S
	0b00001111, // = t
	0b00001001  // = =
};

volatile uint8_t dispBuffer[4];
volatile uint8_t dispCurDigit = 0;

volatile uint_fast16_t aNumber = 0;

void dispDigits(int_fast16_t num);

void main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	// Initialize all ports by setting them to outputs, low
	P1DIR = 0xFF;
	P1OUT = 0;
	P2DIR = 0xFF;
	P2OUT = 0;
	P3DIR = 0xFF;
	P3OUT = 0;

	BCSCTL1 = CALBC1_16MHZ; // Running at 16 MHz
	DCOCTL = CALDCO_16MHZ;

	BCSCTL1 |= DIVA_3; 	// Divide ACLK by 8
	BCSCTL3 |= XCAP_3; 	// Set 12.5pF for crystal

	// Timer for display refresh
	TA0CCR0 = 	DLY_512TH; 			// 1/512s delay time
	TA0CCTL0 = 	CCIE; 				// Perform interrupt when time reached
	TA0CTL |= 	TASSEL_1 | MC_1; 	// Use ACLK, count up mode

	// Timer for count up display
	TA1CCR0 = 	DLY_1S; 			// 1s delay time
	TA1CCTL0 = 	CCIE; 				// Perform interrupt when time reached
	TA1CTL |= 	TASSEL_1 | MC_1; 	// Use ACLK, count up mode

	uint_fast8_t count;

	for (count = 4; count; --count)
		dispBuffer[count] = 0x00;	// Initialize array

	dispDigits(aNumber);

	__enable_interrupt();	// Enable interrupts

	while (1) {
		__bis_SR_register(LPM3 | GIE);	// Set LPM3, allow intterupts on ACLK
	}
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0(void) {
	dispCurDigit &= 0b00000011; // Make sure value doesn't exceed 3

	P1OUT = 0x00;	// Clear P1
	P2OUT = 0x0F;	// Set P2.0-2.3 high

	// Only set cathode for digit being displayed to low
	P2OUT &= ~((0b1000 >> dispCurDigit) | 0xF0);
	// Display digit
	P1OUT = dispBuffer[dispCurDigit];

	dispCurDigit++;
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer_A1(void) {
	aNumber++;
	if (aNumber > 9999)
		aNumber = 0;

	uint_fast8_t count;
	uint_fast16_t num = aNumber;

	for (count = 4; count; --count) {
		dispBuffer[count - 1] = segNums[(num%10)];
		num = num/10;
	}
}

void dispDigits(int_fast16_t num) {
	uint_fast8_t count;

	if (num >= 0) {
		for (count = 4; count; --count) {
			dispBuffer[count - 1] = segNums[(num%10)];
			num = num/10;
		}
	} else {
		dispBuffer[0] = 0x01; 	// Minus sign as first character
		num = num * -1;			// Need to turn into positive number now
		for (count = 3; count; --count) {
			dispBuffer[count] = segNums[(num%10)];
			num = num/10;
		}
	}
}
