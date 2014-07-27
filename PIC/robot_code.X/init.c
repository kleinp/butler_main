/* init.c
 * Peter Klein
 * Created on March 7, 2013, 5:40 PM
 * Description:
 *
 */

#include "system.h"
#include "lsm330dlc.h"
#include "uart.h"
#include "messaging.h"
#include "nvm.h"
#include "robot.h"

void initIO(void)
{
   // Disable analog pin functionality
   ANSELB = 0x00;
   ANSELC = 0x00;
   ANSELD = 0x00;
   ANSELE = 0x00;
   ANSELG = 0x00;
   
   // Configure special pins
   TRISBbits.TRISB12 = 0;     // LED1
   TRISBbits.TRISB13 = 0;     // LED2
   TRISBbits.TRISB14 = 0;     // LED3
   // LSM330DLC (accelerometer/gyro)
   TRISDbits.TRISD5 = 0;      // MOSI_AG
   TRISDbits.TRISD3 = 0;      // CLK_AG
   TRISDbits.TRISD2 = 0;      // CS_A
   TRISDbits.TRISD1 = 0;      // CS_G
   TRISDbits.TRISD6 = 1;      // MISO_A
   TRISDbits.TRISD4 = 1;      // MISO_G
   // UART
   TRISFbits.TRISF5 = 0;      // UART1 RX (bluetooth)
   TRISFbits.TRISF4 = 1;      // UART1 TX (bluetooth)
   // MOTORS
   TRISDbits.TRISD0 = 0;      // LM_A
   TRISFbits.TRISF3 = 0;      // LM_B
   TRISFbits.TRISF6 = 0;      // RM_A
   TRISFbits.TRISF2 = 0;      // RM_B
   LATDbits.LATD7 = 0;        // nSLEEP (0 = sleep, 1 = active)
   TRISDbits.TRISD7 = 0;      // nSLEEP
   TRISFbits.TRISF0 = 1;      // FAULT
   // ENCODER
   TRISFbits.TRISF1 = 1;      // ENC_L
   TRISDbits.TRISD8 = 1;      // DIR_L
   TRISEbits.TRISE0 = 1;      // ENC_R
   TRISDbits.TRISD10 = 1;     // DIR_R
   TRISDbits.TRISD9 = 0;      // DIR_L_OUT
   TRISDbits.TRISD11 = 0;     // DIR_R_OUT
   // Debug schedule task execution
   #ifdef TIMING_TRACE
   TRISGbits.TRISG6 = 0;      // D0
   TRISEbits.TRISE7 = 0;      // D1
   TRISEbits.TRISE6 = 0;      // D2
   TRISEbits.TRISE5 = 0;      // D3
   TRISBbits.TRISB2 = 0;      // A0
   TRISBbits.TRISB3 = 0;      // A1
   TRISBbits.TRISB4 = 0;      // A2
   TRISBbits.TRISB5 = 0;      // A3
   D0 = D1 = D2 = D3 = A0 = A1 = A2 = A3 = 0;
   #endif

   led1Off();
   led2Off();
   led3Off();

   // Unlock the pin configuration registers
   __builtin_write_OSCCONL(OSCCON & 0xBF);

   RPOR9bits.RP101R = 1;            // UART1 TX
   RPINR18bits.U1RXR = 100;         // UART1 RX

   RPOR2bits.RP69R = 0b000101;      // MOSI_AG (SPI1) Note no MOSI for SPI3
   RPINR29bits.SDI3R = 70;          // MISO A
   RPINR20bits.SDI1R = 68;          // MISO G
   RPOR1bits.RP67R = 0b000110;      // CLK_AG (SPI1) Note no CLK for SPI3

   RPOR0bits.RP64R = 0b010000;      // LM_A
   RPOR8bits.RP99R = 0b010001;      // LM_B
   RPOR10bits.RP102R = 0b010010;    // RM_A
   RPOR8bits.RP98R = 0b010011;      // RM_B

   RPINR14bits.QEA1R = 97;          // QEA1 Counts
   RPINR14bits.QEB1R = 72;          // QEB1 Direction
   RPINR16bits.QEA2R = 80;          // QEA2 Counts
   RPINR16bits.QEB2R = 74;          // QEB2 Direction

   // Lock the pin configuration registers
   __builtin_write_OSCCONL(OSCCON | 0x40);

}

void initPeripherals1(void)
{
   u1Init(460800, NO_PARITY, ONE_STOP_BIT);
   initMotorPWM();
   //crcInitEngine();
   nvmInit();

   // Set up CN interrupt for SW1
   CNENEbits.CNIEE2 = 1;
   IPC4bits.CNIP = 0x02;
   IFS1bits.CNIF = 0;
   IEC1bits.CNIE = 1;

   // Quadrature encoders
   QEI1CONbits.CCM = 0b01;       // count with external Up/down
   QEI1IOCbits.QFDIV = 0b111;    // 1:256 clock divide for filter
   QEI1IOCbits.FLTREN = 1;       // enable digital filter
   QEI1CONbits.QEIEN = 1;        // enable the module

   QEI2CONbits.CCM = 0b01;       // count with external Up/down
   QEI2IOCbits.QFDIV = 0b111;    // 1:256 clock divide for filter
   QEI2IOCbits.FLTREN = 1;       // enable digital filter
   QEI2CONbits.QEIEN = 1;        // enable the module 
}