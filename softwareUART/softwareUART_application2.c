/*
Modify the full-duplex sample code to a half-duplex UART that receives characters 0 or 1 from the PC. 
Turn on the green LED if a 1 is received, the red LED if a 0 is received, and no LED for other characters. 
Use 4800 baud, 8-bit of data, and 2 stop bits. 
*/

#include "msp430.h"

#define UART_RXD 0x04 // RXD on P1.2 (Timer0_A.CCI1A)
#define UART_TBIT_DIV_2 1000000 / (4800 * 2)
#define UART_TBIT 1000000 / (4800) // Transmission time per bit = clock/baud rate

#define LED_RED 0x01   // P1.0 - Red LED
#define LED_GREEN 0x40 // P1.6 - Green LED

unsigned char rxBuffer; // Received UART character

void configWDT(void);
void configClocks(void);
void configP1_UART(void);
void configLEDs(void);
void TimerA_UART_init(void);
void updateLEDs(unsigned char receivedChar);

void main(void) {
    configWDT();
    configClocks();
    configP1_UART();
    configLEDs();
    __enable_interrupt();

    TimerA_UART_init();
    
    for (;;) {
        // Wait for incoming character
        __bis_SR_register(LPM0_bits); // Waken by Timer_A1_ISR

        // Process the received character
        updateLEDs(rxBuffer);
    }
}

void configWDT(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
}

void configClocks(void) {
    BCSCTL1 = CALBC1_1MHZ;    // Set DCO to 1 MHz for SMCLK
    DCOCTL = CALDCO_1MHZ;
    BCSCTL3 |= LFXT1S_2;      // Set VLO as the source for ACLK (~12 kHz)
}

void configP1_UART(void) {
    P1OUT = 0x00;             // Initialize all GPIO
    P1SEL = UART_RXD;         // Use RXD pin
    P1DIR = 0xFF & ~UART_RXD; // Set pin as input
}

void configLEDs(void) {
    P1DIR |= LED_RED + LED_GREEN; // Set LED pins as outputs
    P1OUT &= ~(LED_RED + LED_GREEN); // Turn off LEDs initially
}

void TimerA_UART_init(void) {
    TA0CCTL1 = SCS + CM1 + CAP + CCIE; // Sync, neg edge, capture, interrupt
    TA0CTL = TASSEL_2 + MC_2;          // SMCLK, continuous mode
}

void updateLEDs(unsigned char receivedChar) {
    P1OUT &= ~(LED_RED + LED_GREEN); // Turn off both LEDs

    if (receivedChar == '0') {
        P1OUT |= LED_RED;  // Turn on red LED
    } else if (receivedChar == '1') {
        P1OUT |= LED_GREEN; // Turn on green LED
    }
}

#pragma vector = TIMER0_A1_VECTOR // RXD interrupt
__interrupt void Timer_A1_ISR(void) {
    static unsigned char rxBitCnt = 8;
    static unsigned char rxData = 0;

    switch (__even_in_range(TA0IV, TA0IV_TAIFG)) {
        case TA0IV_TACCR1: // TACCR1 CCIFG - UART RXD
            TA0CCR1 += UART_TBIT; // Set TACCR1 for next interrupt

            if (TA0CCTL1 & CAP) { // On start bit edge
                TA0CCTL1 &= ~CAP; // Switch to compare mode
                TA0CCR1 += UART_TBIT_DIV_2; // To middle of D0
            } else { // Get next data bit
                rxData >>= 1;
                if (TA0CCTL1 & SCCI) { // Get bit from latch
                    rxData |= 0x80; //1000 0000
                }

                rxBitCnt--;
                if (rxBitCnt == 0) { // All bits RXed?
                    rxBuffer = rxData; // Store in global
                    rxBitCnt = 8;      // Reload bit counter
                    TACCTL1 |= CAP;    // Switch to capture
                    __bic_SR_register_on_exit(LPM0_bits); // Clear LPM0 bits from 0(SR)
                }
            }
            break;
    }
}
