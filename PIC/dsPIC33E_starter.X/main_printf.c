/* main.c
 * Peter Klein
 * Created on March 3, 2013, 9:01 PM
 * Description:
 *
 */

#include "system.h"
#include "init.h"
#include "uart.h"
#include "basicIO.h"

/** Configuration bits *********************************************/
// See p33EP512MU810.h, lines 57021-57382
_FGS(GWRP_OFF & GSS_OFF & GSSK_OFF);
_FAS(AWRP_OFF & APL_OFF & APLK_OFF);
_FOSCSEL(FNOSC_FRC & IESO_OFF);
_FOSC(POSCMD_XT & OSCIOFNC_ON & IOL1WAY_OFF & FCKSM_CSECME);
_FWDT(WDTPOST_PS1 & WDTPRE_PR32 & PLLKEN_ON & WINDIS_OFF & FWDTEN_OFF);
_FPOR(FPWRT_PWR8 & BOREN_ON & ALTI2C1_OFF);
_FICD(ICS_PGD2 & RSTPRI_PF & JTAGEN_OFF);

int main (void)
{
   char *ptr = "Hello world!";
	char *np = 0;
	int i = 5;
   unsigned char v = 0;
   unsigned long t = 456345;
   long long u = -876543210;
   unsigned long long w = 250;
   double x = -321.2345678;

   initIO();
   changeClockFreq(140);
   u1Init(115200, NO_PARITY, ONE_STOP_BIT);  
   
   // Set up CN interrupt for SW3
   CNENDbits.CNIED13 = 1;
   IPC4bits.CNIP = 0x02;
   IEC1bits.CNIE = 1;
   IFS1bits.CNIF = 0;

   // Timer for blinking LED
   T1CONbits.TON = 0;
   T1CONbits.TCS = 0;
   T1CONbits.TGATE = 0;
   T1CONbits.TCKPS = 3;
   TMR1 = 0x00;
   PR1 = 45573;
   IPC0bits.T1IP = 0x01;
   IFS0bits.T1IF = 0;
   IEC0bits.T1IE = 1;
   T1CONbits.TON = 1;

   // Done with configuration output pin goes high
   LATGbits.LATG14 = 1;
   
	printF("%s\n\r", ptr);
   printF(".. And hello to you too! (I give you a 95%% chance of fun)\n\r");
	printF("%s is a null pointer\n\r", np);
	printF("%i = 5\n\r", i);
   printF("%c = 123\n\r",(char)123);
   printF("%+ul = +456345\n\r",t);
   printF("%012g = -00876543210\n\r",u);
   printF("%0+4uc = +0000\n\r",v);
   printF("0x%uXg = 250 (in hex)\n\r",w);
   printF("%0+4.4d = +0003.0141\n\r",3.0141);
   printF("|%-5.5d| = |-321.23456  |\n\r",x);
   printF("|%-5i| = |-1234 |\n\r",-1234);
   printF("|%-5i| = |1234 |\n\r",1234);

   while(1)
   {
      uPutChar(uGetChar(U1),U1);
   }
   
   return 0;
}

void __attribute__((__interrupt__,no_auto_psv)) _T1Interrupt(void)
{
   led1Toggle();
   IFS0bits.T1IF = 0;
}

// Resets the microcontroller if SW3 is pressed
void __attribute__((__interrupt__,no_auto_psv)) _CNInterrupt(void)
{
   if (!PORTDbits.RD13)
      reset();
   IFS1bits.CNIF = 0;
}