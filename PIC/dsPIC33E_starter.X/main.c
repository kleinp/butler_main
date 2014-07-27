/* main.c
 * Peter Klein
 * Created on March 3, 2013, 9:01 PM
 * Description:
 *
 */

#include "system.h"
#include "init.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mmri.h"
#include "uart.h"

/** Configuration bits *********************************************/
// See p33EP512MU810.h, lines 57021-57382
_FGS(GWRP_OFF & GSS_OFF & GSSK_OFF);
_FAS(AWRP_OFF & APL_OFF & APLK_OFF);
_FOSCSEL(FNOSC_FRC & IESO_OFF);
_FOSC(POSCMD_XT & OSCIOFNC_ON & IOL1WAY_OFF & FCKSM_CSECME);
_FWDT(WDTPOST_PS1 & WDTPRE_PR32 & PLLKEN_ON & WINDIS_OFF & FWDTEN_OFF);
_FPOR(FPWRT_PWR8 & BOREN_ON & ALTI2C1_OFF);
_FICD(ICS_PGD2 & RSTPRI_PF & JTAGEN_OFF);

int main(void)
{
   initIO();
   changeClockFreq(128); // nice and round 2500 timer counts to get 100Hz

   initPeripherals0();
   mmriInit();

   printf("\nRunning...\n");

   while (1)
   {
      mmriMsgHandler();
   }

   return 0;
}

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
   led1Toggle();
   IFS0bits.T1IF = 0;
}

// Resets the microcontroller if SW3 is pressed

void __attribute__((__interrupt__, no_auto_psv)) _CNInterrupt(void)
{
   if (!PORTDbits.RD13)
      reset();
   IFS1bits.CNIF = 0;
}