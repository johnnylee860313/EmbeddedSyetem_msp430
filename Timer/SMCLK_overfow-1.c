#include <msp430.h> 


/**
 * Timer01.c
 * Use SMCLK clock at 800KHZ
 * When TAR (Timer A register, 16 bits) overflows , it has counted 2^16, (2^16/800KHZ ~= 0.08sec)
 * Dive the frequency of the clock by 8 to give a period of about 0.66sec
 */
#define LED1 BIT6
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	P1OUT = ~LED1;
	P1DIR = LED1;
	/*set up timer A :
	 * MC_2 : mode 2 for continuous counting up
	 * ID_3 : timer frequency divider (8)
	 * TASSEL : Timer A selector for SMCLK
	 * TACLR : clear Timer A
	*/
	TACTL = MC_2 | ID_3 | TASSEL_2 | TACLR;
	
	for(;;) {
	    while(!(TACTL & TAIFG)); //wait for overflow, do nothing, concept of polling
	    TACTL &= ~TAIFG; //clear the overflow flag
	    P1OUT ^= LED1; //toggle LED
	}
	return 0;
}
