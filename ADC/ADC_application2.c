/*
Enable button interrupt. 
Every time the button is pushed, measure the temperature of MSP430. 
If the temperature is higher than 737, turn on the red light for a second. 
Otherwise, turn on the green light for a second. 
*/
#include <msp430g2553.h>

void ConfigWDT(void);
void ConfigClocks(void);
void ConfigLEDs(void);
void ConfigADC10(void);
void ConfigButton(void);

volatile unsigned int temperature = 0;  // Store the measured temperature

int main(void) {
    ConfigWDT();         // Stop watchdog timer
    ConfigClocks();      // Configure clocks (DCO as MCLK, VLO as ACLK)
    ConfigLEDs();        // Set up LEDs
    ConfigADC10();       // Set up ADC10 for temperature sensing
    ConfigButton();      // Configure button for interrupts
    __bis_SR_register(GIE);  // Enable global interrupts

    while (1) {
        // Main loop does nothing, everything is handled in the ISR.
        __no_operation(); // Put the MCU in a low-power mode if desired.
    }
}

// 1. Stop the watchdog timer
void ConfigWDT(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer
}

// 2. Configure clocks
void ConfigClocks(void) {
    BCSCTL1 = CALBC1_1MHZ;  // Set DCO to 1 MHz
    DCOCTL = CALDCO_1MHZ;
    BCSCTL3 |= LFXT1S_2;    // Set VLO as the source for ACLK (~12 kHz)
}

// 3. Configure LEDs
void ConfigLEDs(void) {
    P1DIR |= BIT0 | BIT6;  // Set P1.0 (Red LED) and P1.6 (Green LED) as outputs
    P1OUT &= ~(BIT0 | BIT6);  // Turn off both LEDs initially
}

// 4. Configure ADC10 for temperature sensing
void ConfigADC10(void) {
    ADC10CTL1 = INCH_10 | ADC10DIV_3;  // Measure the internal temperature sensor, clock divided by 4
    ADC10CTL0 = SREF_1 | ADC10SHT_2 | REFON | ADC10ON;  // Use internal reference, sample time 16xADC10CLK, turn on ADC10
}

// 5. Configure button interrupt
void ConfigButton(void) {
    P1DIR &= ~BIT3;     // Set P1.3 as input (Button)
    P1REN |= BIT3;      // Enable pull-up/down resistor
    P1OUT |= BIT3;      // Set pull-up resistor
    P1IE |= BIT3;       // Enable interrupt for P1.3
    P1IFG &= ~BIT3;     // Clear interrupt flag for P1.3
}

// 6. Button Interrupt Service Routine
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void) {
    ADC10CTL0 |= ENC | ADC10SC;  // Start ADC10 conversion
    while (ADC10CTL1 & ADC10BUSY);  // Wait until conversion is complete
    temperature = ADC10MEM;  // Read the measured temperature

    if (temperature > 737) {
        P1OUT |= BIT0;  // Turn on Red LED
        __delay_cycles(1000000);  // Keep Red LED on for 1 second (1 MHz clock)
        P1OUT &= ~BIT0;  // Turn off Red LED
    } else {
        P1OUT |= BIT6;  // Turn on Green LED
        __delay_cycles(1000000);  // Keep Green LED on for 1 second (1 MHz clock)
        P1OUT &= ~BIT6;  // Turn off Green LED
    }

    P1IFG &= ~BIT3;  // Clear the interrupt flag for P1.3
}
