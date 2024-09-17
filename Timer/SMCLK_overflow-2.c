#include <msp430.h>


/**
 * Timer02.c
 * Use SMCLK clock at 800KHZ
 * When TAR (Timer A register, 16 bits) overflows it has counted to TACCR0
 * Dive the frequency of the clock by 8 to 100KHZ, 50000 counts for delay of 0.5 sec
 */
#define LED1 BIT0
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    P1OUT = ~LED1;
    P1DIR = LED1;
    /*set up timer A :
     * MC_1 : mode 2 for counting up to TACCR0
     * ID_3 : timer frequency divider (8)
     * TASSEL : Timer A selector for SMCLK
     * TACLR : clear Timer A
    */
    TACCR0 = 49999;
    TACTL = MC_1 | ID_3 | TASSEL_2 | TACLR;

    for(;;) {
        while(!(TACTL & TAIFG)); //wait for overflow, do nothing, concept of polling
        TACTL &= ~TAIFG; //clear the overflow flag
        P1OUT ^= LED1; //toggle LED
    }
    return 0;
}
