/*
 * SMCLK_DCO.c
 *
 *  Created on: 2024/10/1
 *  Author : Johny Lee
 *  Flash green LED at 1 Hz by polling Timer_A, which is driven by SMCLK sourced by DCO running at 1 MHz.
 */

#include <msp430.h>

#define GREEN_LED BIT6 // P1.6 Green LED
#define RED_LED BIT0 // P1.6 Green LED

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop the Watchdog timer
    BCSCTL1 = CALBC1_1MHZ;      // Set DCO to 1 MHz
    DCOCTL = CALDCO_1MHZ;       //DCOCTL and BCSCTL1 combined define the frequency of DCO, among other settings

    P1DIR |= GREEN_LED + RED_LED;         // Set P1.6 as output (Green LED)
    P1OUT &= ~GREEN_LED;        // Turn off the Green LED initially
    P1OUT &= ~RED_LED;

    // Configure Timer_A
    TACTL = TASSEL_2 + MC_1 + ID_3;  // SMCLK, up mode, input divider /8
    TACCR0 = 62500 - 1;              // 1MHz / 8 = 125kHz; 62500/125kHz = 0.5Hz (so it toggles 2 times every 1 second for 0.5Hz)

    while (1)
    {
        // Polling Timer_A
        if (TACTL & TAIFG) // Check Timer_A overflow flag
        {
            TACTL &= ~TAIFG; // Clear flag
            P1OUT ^= (GREEN_LED+RED_LED); // Toggle Green LED
        }
    }
}
