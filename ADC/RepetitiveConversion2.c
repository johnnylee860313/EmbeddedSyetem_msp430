/*
Continuous sampling driven by Timer_A
A1 is sampled 16/second (ACLK/2048) with reference to 1.5V, where ACLK runs at 32 KHz driven by an external crystal. 
If A1 > 0.5Vcc, P1.0 is set, else reset. 
Timer_A is run in up mode and its CCR1 is used to automatically trigger ADC10 conversion, while CCR0 defines the sampling period
Use internal oscillator times sample (16x) and conversion (13x). 
*/
#include <sp430g2553.h>

void main(void){
    WDTCTL = WDTPW + WDTHOLD;
    /*set ADC10*/
    ADC10CTL0 = SREF1 + ADC10SHT_2 + REFON + ADC10ON + ADC10IE; 
    ADC10CTL1 = SHS_1 + CONSEQ_2 + INCH_1;
    
    /*Set timer, Due to ref voltage settle in msp430 will be stable after 30  microsec */
    __enable_interrupt();
    TACCR0 = 30;
    TACCTL0 |= CCIE;
    TACTL = TASSEL_2 + MC_1;       //SMCLK, up mode, smclk clock sourced default is DCO ~= 1MHz , so count up to 30 ~= 30 microsec
    LPM0;                           // Low Power Mode, wait for settle
    TACCTL0 &= ~CCIE;
    __disable_interrupt();

    /*Enable ADC10*/
    ADC10CTL0 |= ENC;
    ADC10AE0 |= 0x02;               //Enable adc on P1.1, or it will be viewed as gpio

    P1DIR |= 0x01;                  // Set P1.0 led output
    TACCR0 = 2048;                  // Sampling period
    
    /*
    TACCR1 set/reset
    Timer_A CCR1 out mode 3: The output (OUT1) is set when the timer counts to the TACCR1 value. 
    It is reset when the timer counts to the TACCR0 value.
    So below will sample and hold the no.2047 clock adc value then reset when count to TACCR0
    */
    TACCTL1 = OUTMOD_3;  
    TACCR1 = 2047;                  // TACCR1 OUT1 on time
    TACTL = TASSEL_1 + MC_1;        // ACLK, up mode

    // Enter LPM3 w/ interrupts
    __bis_SR_register(LPM3_bits + GIE);
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void){
    if (ADC10MEM < 0x155)           // ADC10MEM = A1 > 0.5V?
        P1OUT &= ~0x01;             // Clear P1.0 LED off
    else
        P1OUT |= 0x01;              // Set P1.0 LED on
}

#pragma vector=TIMERA0_VECTOR
__interrupt void ta0_isr(void){
    TACTL = 0;
    LPM0_EXIT;                        // Exit LPM0 on return
}

/*
Explanation of ADC10CTL0 Flags
ADC10CTL0 is a control register that manages various settings for the ADC10 module. Here is the meaning of each flag in this line:

SREF_1: This sets the reference voltage source for the ADC.

SREF_1 selects VREF+ (internal reference voltage) as V+ and AVss as V-.
The internal reference voltage can be 1.5V or 2.5V, depending on other settings (not shown in this snippet).
ADC10SHT_2: This sets the sample-and-hold time for the ADC.

ADC10SHT_2 means the sample-and-hold time is 16 × ADC10CLK.
The sample-and-hold time is the duration the ADC samples the input signal before conversion. Longer times can improve accuracy when dealing with signals that have slow settling times.
REFON: This turns on the internal reference voltage generator.

When REFON is set, the internal reference voltage (VREF) is activated and used as specified by the SREF settings.
It is required if using the internal reference voltage for conversions.
ADC10ON: This enables the ADC10 module.

The ADC10ON flag must be set to power up the ADC and prepare it for conversions.
It does not start a conversion by itself but simply makes the ADC operational.
ADC10IE: This enables interrupts for the ADC10.

When ADC10IE is set, an interrupt will be triggered upon the completion of a conversion.
The interrupt service routine can be used to handle the result of the ADC conversion.
Explanation of ADC10CTL1 Flags
ADC10CTL1 is another control register that sets additional configuration parameters for the ADC10. Here’s what each flag means:

SHS_1: This sets the sample-and-hold source.

SHS_1 means the ADC sample-and-hold operation will be triggered by Timer_A.OUT1.
The ADC will start a sample-and-hold process when it receives a signal from Timer_A.OUT1, making it suitable for synchronized sampling with a timer.
CONSEQ_2: This sets the conversion sequence mode.

CONSEQ_2 configures the ADC to operate in repeat-single-channel mode.
In this mode, the ADC will repeatedly sample and convert a single channel (specified by INCH_1) until manually stopped.
INCH_1: This selects the input channel for the ADC.

INCH_1 corresponds to A1 (analog input channel 1).
The ADC will convert the voltage present at the A1 input pin.
What Clock Will the ADC10 Run At?
By default, the ADC10 module operates using the ADC10OSC (an internal clock dedicated to the ADC), which runs at about 5 MHz.
If a different clock source is required (e.g., using ADC10SSEL_x to select a clock like ACLK, MCLK, or SMCLK), the code must specify it in the ADC10CTL1 register. However, in this code, no such selection is made, so the default ADC10OSC is used.
Summary of Configuration
Reference Voltage: Internal reference voltage (VREF+, selected by SREF_1) with AVss as the negative reference.
Sample-and-Hold Time: 16 ADC10CLK cycles (ADC10SHT_2).
Internal Reference: Enabled (REFON).
ADC Power: Enabled (ADC10ON).
Interrupts: Enabled (ADC10IE).
Trigger Source: Timer_A.OUT1 (SHS_1).
Conversion Mode: Repeat-single-channel (CONSEQ_2), continuously converting input channel A1 (INCH_1).
Flow of Operation
The ADC uses the internal reference voltage.
It waits for a trigger from Timer_A.OUT1 to start each sampling.
The sample-and-hold time is set to 16 ADC10CLK cycles.
The ADC continuously samples from A1 and triggers an interrupt upon each conversion completion.
The interrupt allows handling the results in an interrupt service routine (ISR).

*/