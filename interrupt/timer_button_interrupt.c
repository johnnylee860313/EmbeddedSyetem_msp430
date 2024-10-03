#include <msp430g2553.h>

void ConfigWDT(void);
void ConfigClocks(void);
void ConfigLEDs(void);
void ConfigTimerA2(void);
void ConfigButton(void);

// Configurations for the Watchdog Timer
void ConfigWDT(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Stop the Watchdog Timer
}

// Configurations for the clocks
void ConfigClocks(void) {
    BCSCTL3 |= LFXT1S_2;       // Set VLO as the source 
    BCSCTL2 |= SELS;           // Set SMCLK source as VLO (~ 12kHz)
}

// Configurations for the LEDs
void ConfigLEDs(void) {
    P1DIR |= BIT0 | BIT6;      // Set P1.0 and P1.6 as output (Red and Green LEDs)
    P1OUT &= ~(BIT0 | BIT6);   // Turn off both LEDs initially
}

// Configurations for the button on P1.3
void ConfigButton(void) {
    P1DIR &= ~BIT3;            // Set P1.3 as input (Button)
    P1REN |= BIT3;             // Enable pull-up/down resistor on P1.3
    P1OUT |= BIT3;             // Set P1.3 as pull-up resistor
    P1IE |= BIT3;              // Enable interrupt for P1.3
    P1IES |= BIT3;             // Set interrupt to trigger on falling edge (button press)
    P1IFG &= ~BIT3;            // Clear the interrupt flag for P1.3
}

// Configurations for Timer_A
void ConfigTimerA2(void) {
    TACCTL0 = CCIE;            // Enable Timer_A interrupt for CCR0
    TACCR0 = 5999;             // 12kHz / 6000 = 2Hz, so toggle every 0.5s (1Hz blink rate)
    TACTL = TASSEL_1 | MC_1 | TACLR;  // Source Timer_A from ACLK (VLO), up mode, clear TAR
}

// Main function
void main(void) {
    ConfigWDT();               // Stop the watchdog timer
    ConfigClocks();            // Configure clocks (SMCLK sourced by VLO)
    ConfigLEDs();              // Set up LEDs
    ConfigButton();            // Set up the button interrupt
    ConfigTimerA2();           // Configure Timer_A
    _BIS_SR(GIE);              // Enable global interrupts

    while(1) {
        // Main loop can optionally put the MCU in low-power mode
        // Red LED is controlled by the button interrupt
    }
}

// Timer_A Interrupt Service Routine for CCR0
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
    P1OUT ^= BIT6;             // Toggle Green LED (P1.6) at 1 Hz
}

// Port 1 Interrupt Service Routine for the button
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void) {
    if (P1IFG & BIT3) {        // Check if the interrupt was caused by P1.3 (Button)
        if (!(P1IN & BIT3)) {  // If the button is pressed (low logic level)
            P1OUT |= BIT0;     // Turn on Red LED (P1.0)
        } else {               // If the button is released (high logic level)
            P1OUT &= ~BIT0;    // Turn off Red LED (P1.0)
        }
        P1IFG &= ~BIT3;        // Clear the interrupt flag for P1.3
    }
}
