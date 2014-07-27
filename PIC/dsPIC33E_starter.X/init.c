/* init.c
 * Peter Klein
 * Created on March 7, 2013, 5:40 PM
 * Description:
 *
 */

#include "system.h"
#include "uart.h"

void initIO(void)
{
   // Disable analog pin functionality
   ANSELA = 0x00;
   ANSELB = 0x00;
   ANSELC = 0x00;
   ANSELD = 0x00;
   ANSELE = 0x00;
   ANSELG = 0x00;
   
   // Configure special pins
   TRISDbits.TRISD7 = 1;      // SW1
   TRISDbits.TRISD8 = 1;      // SW2
   TRISDbits.TRISD13 = 1;     // SW3
   
   TRISDbits.TRISD0 = 0;      // LED1
   TRISDbits.TRISD1 = 0;      // LED2

   TRISDbits.TRISD2 = 0;      // UART1 TX (Also LED3)
   TRISDbits.TRISD3 = 1;      // UART1 RX

   TRISGbits.TRISG14 = 0;     // Output for logic analyzer
   
   // Unlock the pin configuration registers
   __builtin_write_OSCCONL(OSCCON & 0xBF);

   RPOR1bits.RP66R = 0b000001;      // UART1 TX
   RPINR18bits.U1RXR = 67;          // UART1 RX

   // Lock the pin configuration registers
   __builtin_write_OSCCONL(OSCCON | 0x40);

}

void initPeripherals0(void)
{
   // Set up CN interrupt for SW3
   CNENDbits.CNIED13 = 1;
   IPC4bits.CNIP = 0x02;
   IEC1bits.CNIE = 1;
   IFS1bits.CNIF = 0;

   // Timer for blinking LED
   T1CONbits.TON = 0;
   T1CONbits.TCS = 0;
   T1CONbits.TGATE = 0;
   T1CONbits.TCKPS = 2;
   TMR1 = 0x00;
   PR1 = getClockFreq()/256/2/10; // Last number is rate in Hz
   IPC0bits.T1IP = 0x01;
   IFS0bits.T1IF = 0;
   IEC0bits.T1IE = 1;
   T1CONbits.TON = 1;

   u1Init(921600, NO_PARITY, ONE_STOP_BIT);
}