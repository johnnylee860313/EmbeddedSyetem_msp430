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