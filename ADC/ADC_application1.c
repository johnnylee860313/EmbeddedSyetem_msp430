/*
Measure the temperature of MSP430 every second using the temperature sensor inside ADC10. Flash the red LED if the temperature rises and the green LED if it drops. Flash both LEDs if the temperature remains unchanged between two consecutive measurements.
The sampling of ADC10 must be triggered continuously by Timer_A.
You can use an infinite loop to flash the LEDs.

And there are some stepsto follow :
1. Close watchdog timer
2. Set DCO as src of MCLK, 1 MHz, and VLO as ACLK
3. Setup both LED lights and set it initially off
4. You can modify the sample code 2 to finish the basic, you can first set timer_A source from MCLK(DCO) to settle 30 micro second delay, and then set timer_A source from ACLK(VLO).
5. Set ADC configuration into: 
5.1 Sample-and-hold source from timer_A
5.2 Temperature sensor channel
5.3 Use ideal reference
5.4 Conversion sequence mode : Repeat-single-channel
5.5 Enable ADC10 interrupt
6. Every second ADC10IFG is set when conversion results(temperature) are loaded into ADC10MEM and invoke ADC ISR.
*/

#include <msp430g2553.h>

void ConfigWDT(void);
void ConfigClocks(void);
void ConfigLEDs(void);
void ConfigTimerA(void);
void ConfigADC10(void);

volatile unsigned int tempPrevious = 0;  // Store previous temperature reading

int main(void) {
    ConfigWDT();        // 1. Close watchdog timer
    ConfigClocks();     // 2. Set DCO to 1 MHz (MCLK) and VLO as ACLK
    ConfigLEDs();       // 3. Configure LEDs
    ConfigTimerA();     // 4. Configure Timer_A
    ConfigADC10();      // 5. Configure ADC10 for temperature sensing
    __bis_SR_register(GIE);  // Enable global interrupts

    while (1) {
        // Main loop does nothing, everything is handled in the ISR.
        __no_operation(); // Put the MCU in a low-power mode if desired.
    }
}

// 1. Close the watchdog timer
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

// 4. Configure Timer_A
void ConfigTimerA(void) {
    //delay for 30 microsec
    __enable_interrupt();
    TACCR0 = 30;
    TACCTL0 |= CCIE;
    TACTL = TASSEL_2 | MC_1 | TACLR; //SMCLK, up mode, smclk clock sourced default is DCO ~= 1MHz , so count up to 30 ~= 30 microsec
    LPM0;                           // Low Power Mode, wait for settle
    TACCTL0 &= ~CCIE;
    __disable_interrupt();

    // After initial setup, switch to ACLK sourced from VLO for 1-second intervals
    __enable_interrupt();
    TACCR1 = 12000;  // 12 kHz ACLK results in 1-second interval (12000 ticks)
    TACCTL1 |= CCIE;
    TACTL = TASSEL_1 | MC_1 | TACLR;  // Use ACLK (VLO ~12 kHz) as Timer_A source, up mode
    LPM0;                           // Low Power Mode, wait for settle
    TACCTL1 &= ~CCIE;
    __disable_interrupt();
}

// 5. Configure ADC10 for temperature sensing
void ConfigADC10(void) {
    // 5.2 Sample-and-hold source from Timer_A, 5.1 Temperature sensor channel, 5.4 Repeat-single-channel
    ADC10CTL1 = INCH_10 | SHS_1 | CONSEQ_2;
    // 5.3 Use ideal reference, sample time 16xADC10CLK, reference on, ADC on, 5.5 Enable ADC10 interrupt
    ADC10CTL0 = SREF_1 | ADC10SHT_2 | REFON | ADC10ON | ADC10IE;
    ADC10CTL0 |= ENC;  // Enable conversion
}

// 6. ADC10 interrupt service routine
#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
    unsigned int tempCurrent = ADC10MEM;  // Read the temperature from ADC10MEM

    if (tempCurrent > tempPrevious) {
        // Temperature increased
        P1OUT |= BIT0;  // Turn on Red LED
        P1OUT &= ~BIT6; // Turn off Green LED
    } else if (tempCurrent < tempPrevious) {
        // Temperature decreased
        P1OUT |= BIT6;  // Turn on Green LED
        P1OUT &= ~BIT0; // Turn off Red LED
    } else {
        // Temperature unchanged
        P1OUT ^= BIT0 | BIT6;  // Toggle both LEDs
    }

    tempPrevious = tempCurrent;  // Store the current reading for the next comparison
    ADC10CTL0 |= ENC | ADC10SC;  // Start the next conversion
}

#pragma vector=TIMERA0_VECTOR
__interrupt void ta0_isr(void){
    TACTL = 0;
    LPM0_EXIT;                        // Exit LPM0 on return
}
