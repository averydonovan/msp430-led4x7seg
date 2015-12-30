/*
 *  A
 * F B
 *  G
 * E C
 *  D  DP
 */

#include <msp430.h>
#include <stdint.h>

#define DLY_1S		4096 - 1
#define DLY_500MS	4096/2 - 1
#define DLY_512TH	7
#define DLY_1024TH	3

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

/*const byte segLets[7] = {
  { 0,0,0,1,1,0,1 }, // = c
  { 0,1,1,1,1,0,1 }, // = d
  { 0,0,1,0,1,1,1 }, // = h
  { 1,0,1,1,0,1,1 }, // = S
  { 0,0,0,1,1,1,1 }, // = t
  { 0,0,0,1,0,0,1 } // = =
};*/

volatile uint8_t dispBuffer[4];
volatile uint8_t dispCurDigit = 0;

volatile uint_fast16_t aNumber = 0;

void dispDigits(int_fast16_t num);

void main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	P1DIR = 0xFF;
	P1OUT = 0;
	P2DIR = 0xFF;
	P2OUT = 0;
	P3DIR = 0xFF;
	P3OUT = 0;

	BCSCTL1 = CALBC1_16MHZ; // Running at 16 MHz
	DCOCTL = CALDCO_16MHZ;

	BCSCTL1 |= DIVA_3; // Divide clock by 8
	BCSCTL3 |= XCAP_3; // Set 12.5pF for crystal

	//TACCR0 = 	DLY_500MS; 	// 500ms delay time
	TA0CCR0 = 	DLY_512TH; 	// 1/512s delay time
	TA0CCTL0 = 	CCIE; 		// Perform interrupt when time reached
	// Use ACLK, count up mode, and divide clock by 8 again
	TA0CTL |= 	TASSEL_1 | MC_1; //| ID_3;

	TA1CCR0 = 	DLY_1S; 	// 1/512s delay time
	TA1CCTL0 = 	CCIE; 		// Perform interrupt when time reached
	TA1CTL |= 	TASSEL_1 | MC_1; //| ID_3;

	uint_fast8_t count;

	for (count = 4; count; --count)
		dispBuffer[count] = 0x00;

	dispDigits(aNumber);

	__enable_interrupt();

	while (1) {
		__bis_SR_register(LPM3 | GIE);
	}
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0(void) {
	dispCurDigit &= 0b00000011;

	P1OUT = 0x00;
	P2OUT = 0x0F;

	P2OUT &= ~((0b1000 >> dispCurDigit) | 0xF0);
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
		dispBuffer[0] = 0x01;
		num = num * -1;
		for (count = 3; count; --count) {
			dispBuffer[count] = segNums[(num%10)];
			num = num/10;
		}
	}
}
