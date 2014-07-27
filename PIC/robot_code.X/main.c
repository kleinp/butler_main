/* main.c
 * Peter Klein
 * Created on March 3, 2013, 9:01 PM
 * Description:
 *
 */

#include "system.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "init.h"
#include "uart.h"
#include "lsm330dlc.h"
#include "nvm.h"
#include "mmri.h"
#include "robot.h"

/** Configuration bits *********************************************/
_FGS( GWRP_OFF & GSS_OFF & GSSK_OFF );
_FAS(AWRP_OFF & APL_OFF & APLK_OFF);
_FOSCSEL(FNOSC_FRC & IESO_OFF);
_FOSC(POSCMD_XT & OSCIOFNC_OFF & IOL1WAY_OFF & FCKSM_CSECME);
_FWDT(WDTPOST_PS1 & WDTPRE_PR32 & PLLKEN_ON & WINDIS_OFF & FWDTEN_OFF);
_FPOR(FPWRT_PWR8 & BOREN_OFF & ALTI2C1_OFF);
_FICD(ICS_PGD2 & RSTPRI_PF & JTAGEN_OFF);

int main (void)
{
   char user_input[30] = "";
   char delim[] = ",;";
   uint8_t addr, length, error, uart_echo = U1;
   char *end;
   char *val;

   initIO();  
   changeClockFreq(120);
   initPeripherals1();
   
   mmriInit();
   mmriReadNVM();
   
   mmriInitVar(5, UINT8, RW, VOL, PWWR, &uart_echo);
   lsmInit();
   robotInit();

   while(1)
   {
      printf("\n>");
      length = getString(&user_input[0], 30, U1, uart_echo)-1;

      addr = (uint8_t)strtol(strtok(&user_input[0], delim), &end, 0);
      if (*end != '\0')
      {
         printf(" [BADADDR]\n");
         continue;
      }
      val = strtok(NULL, delim);
      // If user puts 2 question marks after address, print out that address
      // Example: 7,?? prints out contents of register 7
      if (*(val+1) == '?' && *val == '?')
      {
         uPutChar(' ', U1);
         uPutChar('=', U1);
         uPutChar(' ', U1);
         mmriPrintReg(addr);
      }
      else
      {
         error = mmriWriteRegAscii(addr, val);
         if (error)
            printf("\nERROR: %i", error);
      }
   }
        
   return 0;
}

// Resets the microcontroller if SW1 is pressed
void __attribute__((__interrupt__,no_auto_psv)) _CNInterrupt(void)
{
   if (SWITCH1)
      reset();
   IFS1bits.CNIF = 0;
}

