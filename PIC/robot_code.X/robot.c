/* robot.c
 * Peter Klein
 * Created on August 18, 2013, 1:44 PM
 * Description:
 *
 */

#include "system.h"
#include "robot.h"
#include "lsm330dlc.h"
#include "messaging.h"
#include "uart.h"
#include "mmri.h"
#include "math.h"

// mmri variables
encoder32 enc_left, enc_right;
int8_t mot_left, mot_right;

// internal variables
uint16_t oc_val_max = 0;
uint16_t oc_val_div = 0;
int32_t angleI, angleIold, rate, mot_speed;
int32_t div_p = 30;
int32_t div_d = 2000;
int32_t offset = 0;

LSM_DATA lsm_data;

void robotInit()
{
   // Register the encoder values with mmri
   mmriInitVar(6, INT32, RO, VOL, NOPW, &enc_left.val);
   mmriInitVar(7, INT32, RO, VOL, NOPW, &enc_right.val);

   // Disable motor and register speed variables with mmri
   mot_left = 0;        // set motor speeds to 0
   mot_right = 0;
   mmriInitVar(8, INT8, RW, VOL, NOPW, &mot_left);
   mmriInitVar(9, INT8, RW, VOL, NOPW, &mot_right);

   mmriInitVar(10, INT32, RO, VOL, NOPW, &angleI);
   mmriInitVar(11, INT32, RW, VOL, NOPW, &div_p);
   mmriInitVar(12, INT32, RW, VOL, NOPW, &div_d);
   mmriInitVar(13, INT32, RW, VOL, NOPW, &offset);

   // Timer for primary interrupt loop
   T1CONbits.TON = 0;
   T1CONbits.TCS = 0;
   T1CONbits.TGATE = 0;
   T1CONbits.TCKPS = 0b10;
   TMR1 = 0x00;
   PR1 = getClockFreq()/64/2/100; // Last number is rate in Hz
   IPC0bits.T1IP = 0x01;
   IFS0bits.T1IF = 0;
   IEC0bits.T1IE = 1;
   T1CONbits.TON = 1;
}

// Called by timer interrupt every 10ms
void tasks100Hz()
{
   static uint8_t *ptr;
   static uint8_t i;
   static uint8_t checksum;
   static float tmp, angle = 0;

   // Read accelerations and angular rates from LSM
   lsmReadMotionData();

   // Calculate a tilt angle of the robot
   tmp = atan2(lsm_data.az, lsm_data.ax) * 180/3.14159;
   angle = 0.98*(angle + lsm_data.gy/10*0.01) + 0.02*tmp;
   angleI = (int32_t)(angle * 100);

   // Run control code
   rate = (angleI - angleIold)*100;    // dAngle/dT (dT=0.01 -> *100)

   if (abs(angleI) < 6000)
      mot_speed = angleI/div_p - rate/div_d - offset;
   else
      mot_speed = 0;

   mot_right = mot_speed;
   mot_left = mot_speed;
   angleIold = angleI;
   
   // Update pos
   enc_left.val16[0] = POS1CNTL;
   enc_left.val16[1] = POS1CNTH;
   enc_right.val16[0] = POS2CNTL;
   enc_right.val16[1] = POS2CNTH;

   // Update motors
   setMotorOutputs(mot_left, mot_right);

   // Send out a packet
   checksum = 0;

   //printf("A:%+05li R:%+05li M:%05li\n", angleI, rate, mot_speed);
   //printf("A:%f I:%li\n", angle, angleI);

   /*uPutChar(1, U1);              // "Start of heading"
   uPutChar(1, U1);              // "Start of heading"
   ptr = (uint8_t*)&lsm_data.counter;
   checksum+=*ptr;
   uPutChar(*ptr, U1);           // counter
   ptr = (uint8_t*)&lsm_data.ax;
   for (i=0;i<12;i++)
   {
      checksum+=*ptr;
      uPutChar(*ptr++, U1);      // ax, ay, az, gx, gy, gz in 8-bit chunks
   }

   uPutChar(checksum, U1);
   uPutChar(3, U1);              // "End of text"
   uPutChar(3, U1);              // "End of text"
    */

}

// Called by timer interrupt every 50ms
void tasks50Hz()
{
   
}

// Called by timer interrupt every 100ms
void tasks10Hz()
{
   // Distance sensor
}

// Called by timer interrupt every 200ms
void tasks5Hz()
{
   led1Toggle();

   //printf("\n[ENC 0x%X 0x%X 0x%X 0x%X", POS1CNTL, POS1CNTH, POS2CNTL, POS2CNTH);
}

// Called by timer interrupt every 1000ms
void tasks1Hz()
{

}

// 100Hz interrupt. Calls the taskXHz functions as appropriate
void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
   static uint8_t schedule_counter = 0;

   schedule_counter++;

   if (schedule_counter > 99)
      schedule_counter = 0;

   #ifdef TIMING_TRACE
   D0 = 1;
   #endif
   tasks100Hz();
   #ifdef TIMING_TRACE
   D0 = 0;
   #endif
   if (schedule_counter % 2 == 0)
   {
      #ifdef TIMING_TRACE
      D1 = 1;
      #endif
      tasks50Hz();
      #ifdef TIMING_TRACE
      D1 = 0;
      #endif
   }

   if ((schedule_counter - 1) % 10 == 0)
   {
      #ifdef TIMING_TRACE
      D2 = 1;
      #endif
      tasks10Hz();
      #ifdef TIMING_TRACE
      D2 = 0;
      #endif
   }

   if (schedule_counter % 20 == 0)
   {
      #ifdef TIMING_TRACE
      D3 = 1;
      #endif
      tasks5Hz();
      #ifdef TIMING_TRACE
      D3 = 0;
      #endif
   }

   if (schedule_counter == 2)
   {
      #ifdef TIMING_TRACE
      A0 = 1;
      #endif
      tasks1Hz();
      #ifdef TIMING_TRACE
      A0 = 0;
      #endif
   }
   
   IFS0bits.T1IF = 0;
}

void initMotorPWM(void)
{
   oc_val_max = (uint16_t)(getClockFreq()/20000/2);
   oc_val_div = oc_val_max/50;

   // LM_A
   OC1CON1 = 0;                  // clear configuration registers
   OC1CON2 = 0;
   OC1CON1bits.OCTSEL = 0b111;   // Use peripheral clock
   OC1R = oc_val_max;            // 100% duty cycle to start
   OC1RS = oc_val_max;           // Period counter for ~20kHz
   OC1CON2bits.SYNCSEL = 0x1F;   // No sync or trigger source for this motor
   OC1CON1bits.OCM = 0b110;      // Edge aligned PWM

   // LM_B
   OC2CON1 = 0;                  // clear configuration registers
   OC2CON2 = 0;
   OC2CON1bits.OCTSEL = 0b111;   // Use peripheral clock
   OC2R = oc_val_max;            // 100% duty cycle to start
   OC2CON2bits.SYNCSEL = 0x1;    // Syncronize OC2 PWM to OC1 PWM
   OC2CON1bits.OCM = 0b110;      // Edge aligned PWM

   // RM_A
   OC3CON1 = 0;                  // clear configuration registers
   OC3CON2 = 0;
   OC3CON1bits.OCTSEL = 0b111;   // Use peripheral clock
   OC3R = oc_val_max;            // 100% duty cycle to start
   OC3CON2bits.SYNCSEL = 0x1;    // Syncronize OC3 PWM to OC1 PWM
   OC3CON1bits.OCM = 0b110;      // Edge aligned PWM

   // RM_B
   OC4CON1 = 0;                  // clear configuration registers
   OC4CON2 = 0;
   OC4CON1bits.OCTSEL = 0b111;   // Use peripheral clock
   OC4R = oc_val_max;            // 100% duty cycle to start
   OC4CON2bits.SYNCSEL = 0x1;    // Syncronize OC4 PWM to OC1 PWM
   OC4CON1bits.OCM = 0b110;      // Edge aligned PWM
}

// Provide speed -50 -> 50 for each motor
// Speeds less than 0 cause a change in direction
void setMotorOutputs(int8_t left, int8_t right)
{
   // If either motor is to be on, take the H-bridge out of sleep
   // if nothing is running, make it sleep to save power
   if (left || right)
      LATDbits.LATD7 = 1;        // nSLEEP (0 = sleep, 1 = active)
   else
      LATDbits.LATD7 = 0;

   if (left > 50)          // coerce inputs to within +/- 50
      left = 50;
   if (left < -50)
      left =-50;

   if (right > 50)
      right = 50;
   if (right < -50)
      right = -50;

   if (left < 0)
   {
      OC1R = oc_val_max;
      OC2R = oc_val_max - oc_val_div*(uint16_t)fabs(left);
      DIR_L_OUT = 0;
   }
   else
   {
      OC1R = oc_val_max - oc_val_div*(uint16_t)fabs(left);
      OC2R = oc_val_max;
      DIR_L_OUT = 1;
   }

   if (right < 0)
   {
      OC3R = oc_val_max;
      OC4R = oc_val_max - oc_val_div*(uint16_t)fabs(right);
      DIR_R_OUT = 0;
   }
   else
   {
      OC3R = oc_val_max - oc_val_div*(uint16_t)fabs(right);
      OC4R = oc_val_max;
      DIR_R_OUT = 1;
   }
}
