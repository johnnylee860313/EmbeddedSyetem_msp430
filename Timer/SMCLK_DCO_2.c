/*
 * SMCLK_DCO_2.c
 *
 *  Created on: 2024¦~10¤ë1¤é
 *      Author: user
 */


#include <msp430.h>

#define GREEN_LED BIT6  // P1.6 Green LED
#define RED_LED BIT0    // P1.0 Red LED
#define BUTTON BIT3     // P1.3 Button

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop the Watchdog timer
    BCSCTL1 = CALBC1_1MHZ;      // Set DCO to 1 MHz
    DCOCTL = CALDCO_1MHZ;

    P1DIR |= GREEN_LED + RED_LED;  // Set P1.6 and P1.0 as output (Green and Red LEDs)
    P1OUT &= ~(GREEN_LED + RED_LED); // Turn off both LEDs initially
    P1DIR &= ~BUTTON;            // Set P1.3 as input (Button)
    P1REN |= BUTTON;             // Enable pull-up/pull-down resistor on P1.3
    P1OUT |= BUTTON;             // Configure P1.3 with a pull-up resistor

    // Configure Timer_A
    TACTL = TASSEL_2 + MC_1 + ID_3;  // SMCLK, up mode, input divider /8
    TACCR0 = 62500 - 1;              // 1MHz / 8 = 125kHz; 125kHz / 62500 = 2Hz (toggle every 0.5 seconds)

    while (1)
    {
        // Polling Timer_A for Green LED blink
        if (TACTL & TAIFG) // Check Timer_A overflow flag
        {
            TACTL &= ~TAIFG; // Clear flag
            P1OUT ^= GREEN_LED; // Toggle Green LED
        }

        // Check if the button is pressed (active low)
        if (!(P1IN & BUTTON))
        {
            P1OUT |= RED_LED;  // Turn on Red LED
        }
        else
        {
            P1OUT &= ~RED_LED; // Turn off Red LED
        }
    }
}
