/*
 * Copyright (c) 2016, Donovan Smith
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Demonstration of using a 4-character 7-segment LED display and multiple
 * timers.
 *
 * Written for MSP430G2553 but might work for other MSP430G2xxx devices with
 * at least two timers, such as the MSP430G2x53 and MSPG2x13.
 *
 * Written using Code Composer Studio v6.1.
 *
 * Used LED display part KYX-5461AS hooked as follows:
 *   Pin 11 (Segment A) --- P1.6
 *   Pin  7 (Segment B) --- P1.5
 *   Pin  4 (Segment C) --- P1.4
 *   Pin  2 (Segment D) --- P1.3
 *   Pin  1 (Segment E) --- P1.2
 *   Pin 10 (Segment F) --- P1.1
 *   Pin  5 (Segment G) --- P1.0
 *   Pin  3 (Segment DP) -- P1.7
 *   Pin 12 (Digit 1) ----- P2.3
 *   Pin  9 (Digit 2) ----- P2.2
 *   Pin  8 (Digit 3) ----- P2.1
 *   Pin  6 (Digit 4) ----- P2.0
 *
 * Where display device is as follows:
 *   Pins     12 11 10  9  8  7
 *           +-----------------+
 *   Digits  |  1.  2.  3.  4. |  <-- KYX-5461AS
 *           +-----------------+
 *   Pins      1  2  3  4  5  6
 */

#include <msp430.h>
#include <stdint.h>

#define DLY_1S		4096 - 1
#define DLY_512TH	7
#define DLY_1024TH	3
#define DLY_TIMEOUT 10

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

volatile int_fast16_t aNumber = -999;

volatile uint_fast8_t timeout = DLY_TIMEOUT;

void dispDigits(int_fast16_t num);

void main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	// Initialize all ports by setting them to outputs, low
	P1DIR = 0xFF;
	P1OUT = 0;
	P2DIR = 0xFF;
	P2OUT = 0;
	P3DIR = 0xFF;	// P3 doesn't exist on PDIP20 pinout but is on die
	P3OUT = 0;

	P2REN |= BIT5;    // Enable pull-up resistor for P2.5
	P2DIR &= ~BIT5;   // Use P2.5 for button
	P2OUT |= BIT5;

	DCOCTL  = 0;
	BCSCTL1 = CALBC1_16MHZ; // Run at 16 MHz
	DCOCTL  = CALDCO_16MHZ;

	BCSCTL1 |= DIVA_3; 	// Divide ACLK by 8
	BCSCTL3 |= XCAP_3; 	// Set 12.5pF for crystal

	// Timer for display refresh
	TA0CCR0  = DLY_512TH; 			// 1/512s delay time
	TA0CCTL0 = CCIE; 				// Perform interrupt when time reached
	TA0CTL   = TASSEL_1 + MC_1; 	// Use ACLK, count up mode

	// Timer for count up display
	TA1CCR0  = DLY_1S; 				// 1s delay time
	TA1CCTL0 = CCIE; 				// Perform interrupt when time reached
	TA1CTL   = TASSEL_1 + MC_1; 	// Use ACLK, count up mode

	uint_fast8_t count;

	for (count = 4; count; --count)
		dispBuffer[count] = 0x00;	// Initialize array

	while (1) {
		dispDigits(aNumber);

		__bis_SR_register(LPM3_bits + GIE);	// Set LPM3, allow interrupts on ACLK
	}
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0(void) {
	if (!(P2IN & BIT5)) {
		timeout = DLY_TIMEOUT;		// Button pressed, reset timeout
	}


	if (timeout) {					// Only refresh display if not timed out
		dispCurDigit &= 0b00000011; // Make sure value doesn't exceed 3

		P1OUT = 0x00;				// Clear P1
		P2OUT |= 0x0F;				// Set P2.0-2.3 high

		// Only set cathode for digit being displayed to low
		P2OUT &= ~(0b1000 >> dispCurDigit);
		// Display digit
		P1OUT = dispBuffer[dispCurDigit];

		dispCurDigit++;
	} else {						// Timed out so make sure display off
		P1OUT = 0x00;				// Clear P1
		P2OUT &= ~0x0F;				// Set P2.0-2.3 low
	}
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer_A1(void) {
	aNumber++;
	if (aNumber > 9999)
		aNumber = -999;		// Reset timer since past 9999

	if (timeout)
		timeout--;

	__bic_SR_register_on_exit(LPM3_bits);	// Exit LPM3 on interrupt return
}

void dispDigits(int_fast16_t num) {
	uint_fast8_t count;
	uint_fast16_t numU;
	uint_fast8_t offset;

	if (num >= 0) {				// Number is positive, effectively already unsigned
		numU = num;
		offset = 1;
	} else {					// Number is negative
		dispBuffer[0] = 0x01; 	// Minus sign as first character
		numU = ~num + 1; 		// Convert to unsigned int by inverse of one's complement + 1
		offset = 0;
	}

	for (count = 3 + offset; count; count--) {
		// Remainder of number divided by 10 is the last digit of the number
		dispBuffer[count - offset] = segNums[numU%10];	// Display last digit
		// Divide by 10 to effectively move one digit left in number
		numU = numU/10;
	}
}
