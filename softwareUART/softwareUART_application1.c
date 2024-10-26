/*
Software UART, using Timer0_A3, 9600 baud, echo, full duplex, SMCLK at 1MHz
Main loop readies software UART to receive one character and waits in LPM0 with all activities interrupt-driven
TA0CCR0 (TXD) and TA0CCR1 (RXD) may interrupt at any time and in an interleaved way
TA0R keeps an independent time reference, while CCR0 and CCR1 handle time intervals in TXD and RXD
*/
#include "msp430.h"

#define UART_TXD 0x02 // TXD on P1.1 (Timer0_A.OUT0)
#define UART_RXD 0x04 //RXD on P1.2 (Timer0_A.CCI1A)
#define UART_TBIT_DIV_2 1000000 / (9600 * 2)
#define UART_TBIT 1000000 / (9600) //transmition time per bit = clock/baud rate

unsigned int txData;  // UART internal TX variable 
unsigned char rxBuffer; // Received UART character

void TimerA_UART_init(void);
void TimerA_UART_tx(unsigned char byte);
void TimerA_UART_print(char *string);

//Stop the watchdog timer
void configWDT(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer
}

//Configure clocks
void configClocks(void) {
    BCSCTL1 = CALBC1_1MHZ;  // Set DCO to 1 MHz
    DCOCTL = CALDCO_1MHZ;
    BCSCTL3 |= LFXT1S_2;    // Set VLO as the source for ACLK (~12 kHz)
}

void configP1_UART(void){
    P1OUT = 0x00;       // Initialize all GPIO
    P1SEL = UART_TXD + UART_RXD; // Use TXD/RXD pins
    P1DIR = 0xFF & ~UART_RXD; // Set pins to output
}

void main(void){
    configWDT();
    configClocks();
    configP1_UART();
    __enable_interrupt();

    TimerA_UART_init();
    TimerA_UART_print("G2xx3 TimerA UART\r\n");
    TimerA_UART_print("READY.\r\n");
    
    for (;;) {
        // Wait for incoming character
        __bis_SR_register(LPM0_bits); // waken by Timer_A1_ISR
        // Echo received character
        TimerA_UART_tx(rxBuffer);
    }
}

void TimerA_UART_print(char *string) {
    while (*string) TimerA_UART_tx(*string++);
}

void TimerA_UART_init(void){
    TA0CCTL0 = OUT;   // Set TXD idle as '1'
    // Set RXD: sync, neg edge, capture, interrupt
    TA0CCTL1 = SCS + CM1 + CAP + CCIE; // CCIS1 = 0
    TA0CTL = TASSEL_2 + MC_2; // SMCLK, continuous mode
}

void TimerA_UART_tx(unsigned char byte){
    while (TACCTL0 & CCIE); // Ensure last char TX'd

    TA0CCR0 = TA0R;      // Current state of TA counter
    TA0CCR0 += UART_TBIT; // One bit time till 1st bit
    TA0CCTL0 = OUTMOD0 + CCIE; // Set TXD on EQU0, Int
    txData = byte;       // Load char to be TXD
    txData |= 0x100;    // Add stop bit to TXData
    txData <<= 1;       // Add start bit
}

#pragma vector = TIMER0_A0_VECTOR  // TXD interrupt
__interrupt void Timer_A0_ISR(void) {
    static unsigned char txBitCnt = 10;
    TA0CCR0 += UART_TBIT; // Set TACCR0 for next intrpt
    if (txBitCnt == 0) {  // All bits TXed?
        TA0CCTL0 &= ~CCIE;  // Yes, disable intrpt
        txBitCnt = 10;      // Re-load bit counter
    } else {
        if (txData & 0x01) {// Check next bit to TX
            TA0CCTL0 &= ~OUTMOD2; // TX '1’ by OUTMODE0/OUT
        } else {
            TA0CCTL0 |= OUTMOD2; // TX '0‘
        } 
        txData >>= 1;
        txBitCnt--;
    }
}

#pragma vector = TIMER0_A1_VECTOR // RXD interrupt
__interrupt void Timer_A1_ISR(void) {
    static unsigned char rxBitCnt = 8;
    static unsigned char rxData = 0;

    switch (__even_in_range(TA0IV, TA0IV_TAIFG)) { 
        case TA0IV_TACCR1:     // TACCR1 CCIFG - UART RXD
            TA0CCR1 += UART_TBIT;// Set TACCR1 for next int
            
            if (TA0CCTL1 & CAP) { // On start bit edge
                TA0CCTL1 &= ~CAP;   // Switch to compare mode
                TA0CCR1 += UART_TBIT_DIV_2;// To middle of D0
            } else {             // Get next data bit
                rxData >>= 1;
                if (TA0CCTL1 & SCCI) { // Get bit from latch
                    rxData |= 0x80;
                }

                rxBitCnt--;
                if (rxBitCnt == 0) {  // All bits RXed?
                    rxBuffer = rxData;  // Store in global
                    rxBitCnt = 8;       // Re-load bit counter
                    TACCTL1 |= CAP;     // Switch to capture
                    __bic_SR_register_on_exit(LPM0_bits);  //wake up mian loop
                    // Clear LPM0 bits from 0(SR)
                }
            }
            break;
    }
}