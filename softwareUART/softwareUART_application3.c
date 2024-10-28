/*
Temperature Sensing System
MSP430 reads temperature from ADC every second and compare reading with the one sensed in previous second.
If the current temperature is higher, turn on the red LED and send HI to PC
If the current temperature is lower, turn on the green LED and send LO to PC
If the sensed temperature is equal to the first one turn off both LEDs and send IN to PC
Hint:
Use Timer_A alternatively for timing 1 sec and UART
*/

#include "msp430.h"

#define UART_TXD 0x02 // TXD on P1.1 (Timer0_A.OUT0)
#define UART_RXD 0x04 // RXD on P1.2 (Timer0_A.CCI1A)
#define UART_TBIT_DIV_2 1000000 / (4800 * 2)
#define UART_TBIT 1000000 / (4800)

#define LED_RED 0x01   // P1.0 - Red LED
#define LED_GREEN 0x40 // P1.6 - Green LED

unsigned int txData;  // UART internal TX variable 
unsigned int previousTemp = 0, currentTemp = 0; 
unsigned char rxBuffer; // Received UART character

void configWDT(void);
void configClocks(void);
void configP1_UART(void);
void configLEDs(void);
void configADC(void);
void TimerA_UART_init(void);
void TimerA_UART_tx(unsigned char byte);
void TimerA_UART_print(char *string);
void readTemperature(void);
void compareTemperature(void);

void main(void) {
    configWDT();
    configClocks();
    configP1_UART();
    configLEDs();
    configADC();
    __enable_interrupt();

    TimerA_UART_init();
    
    previousTemp = ADC10MEM;  // Store the initial temperature value
    TimerA_UART_print("Temperature Monitoring Start\r\n");
    
    for (;;) {
        __bis_SR_register(LPM0_bits); // Wait for Timer_A1 ISR to wake up
        readTemperature();
        compareTemperature();
    }
}

void configWDT(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
}

void configClocks(void) {
    BCSCTL1 = CALBC1_1MHZ;    // Set DCO to 1 MHz
    DCOCTL = CALDCO_1MHZ;
    BCSCTL3 |= LFXT1S_2;      // Set VLO as the source for ACLK (~12 kHz)
}

void configP1_UART(void) {
    P1OUT = 0x00;             // Initialize all GPIO
    P1SEL = UART_TXD + UART_RXD; // Use TXD/RXD pins
    P1DIR = 0xFF & ~UART_RXD; // Set pins to output
}

void configLEDs(void) {
    P1DIR |= LED_RED + LED_GREEN; // Set LED pins as outputs
    P1OUT &= ~(LED_RED + LED_GREEN); // Turn off LEDs initially
}

void configADC(void) {
    ADC10CTL1 = INCH_10 + ADC10DIV_3; // Temp Sensor ADC10CLK/4
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON; // Internal ref on, ADC on
    __delay_cycles(1000); // Delay for reference to settle
}

void readTemperature(void) {
    ADC10CTL0 |= ENC + ADC10SC; // Sampling and conversion start
    while (ADC10CTL1 & ADC10BUSY); // Wait until conversion is complete
    currentTemp = ADC10MEM; // Read the converted value
}

void compareTemperature(void) {
    if (currentTemp > previousTemp) {
        P1OUT = LED_RED; // Turn on red LED
        TimerA_UART_print("HI\r\n");
    } else if (currentTemp < previousTemp) {
        P1OUT = LED_GREEN; // Turn on green LED
        TimerA_UART_print("LO\r\n");
    } else {
        P1OUT &= ~(LED_RED + LED_GREEN); // Turn off both LEDs
        TimerA_UART_print("IN\r\n");
    }
    previousTemp = currentTemp; // Store current temperature as previous for next comparison
}

void TimerA_UART_init(void) {
    TA0CCTL0 = CCIE;            // Enable Timer0_A interrupt for 1-second timing
    TA0CCR0 = 12000;            // ~1 second interval (using ACLK ~12kHz)
    TA0CTL = TASSEL_1 + MC_1;   // ACLK, up mode

    // Initialize UART
    TA0CCTL1 = SCS + CM1 + CAP + CCIE; // Sync, neg edge, capture, interrupt
    TA0CTL |= TASSEL_2 + MC_2;         // SMCLK, continuous mode
}

void TimerA_UART_print(char *string) {
    while (*string) TimerA_UART_tx(*string++);
}

void TimerA_UART_tx(unsigned char byte) {
    while (TACCTL0 & CCIE); // Ensure last char TX'd

    TA0CCR0 = TA0R;          // Current state of TA counter
    TA0CCR0 += UART_TBIT;    // One bit time till 1st bit
    TA0CCTL0 = OUTMOD0 + CCIE; // Set TXD on EQU0, Int
    txData = byte;           // Load char to be TXD
    txData |= 0x100;         // Add stop bits to TXData (two bits)
    txData <<= 1;            // Add start bit
}

#pragma vector = TIMER0_A0_VECTOR  // 1-second timing interrupt
__interrupt void Timer_A0_ISR(void) {
    __bic_SR_register_on_exit(LPM0_bits); // Wake up main loop
}

#pragma vector = TIMER0_A1_VECTOR // UART RXD interrupt
__interrupt void Timer_A1_ISR(void) {
    // UART RX interrupt logic (not needed for this application)
}
