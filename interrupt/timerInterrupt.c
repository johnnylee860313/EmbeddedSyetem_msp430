#include <msp430g2553.h>

void ConfigWDT(void);
void ConfigClocks(void);
void ConfigLEDs(void);
void ConfigTimerA2(void);

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
    ConfigTimerA2();           // Configure Timer_A
    _BIS_SR(GIE);              // Enable global interrupts

    while(1) {
        // Optionally, do something here or put the MCU to low power mode
        P1OUT |= BIT0;         // Keep the Red LED on (P1.0)
    }
}

// Interrupt Service Routine for Timer_A CCR0
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
    P1OUT ^= BIT6;             // Toggle Green LED (P1.6) on every interrupt (1 Hz blink)
}
