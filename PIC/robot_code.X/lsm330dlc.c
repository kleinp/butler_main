/* lsm330dlc.c
 * Peter Klein
 * Created on April 28, 2013, 7:42 PM
 * Description:
 *
 */

#include "system.h"
#include "lsm330dlc.h"

#include <stdio.h>
#include "uart.h"
#include "messaging.h"
#include "mmri.h"

extern LSM_DATA lsm_data;

static void lsmWriteRegister(char ag, unsigned char addr, unsigned char data)
{
   int delay;

   if (ag == ACCEL)
   {
      lsmCsaLow();
      SPIGBUF = addr;                  // no auto increment, R/W bit is zero
      SPIGBUF = data;                  // data
      SPIABUF = 0x00;                  // needed to have module read
      SPIABUF = 0x00;                  // needed to have module read
      for(delay=0;delay<28;delay++);   // Wait for transmit to finish
      lsmCsaHigh();
      delay = SPIABUF;                 // dummy read
      delay = SPIABUF;                 // dummy read
      delay = SPIGBUF;                 // dummy read
      delay = SPIGBUF;                 // dummy read
      return;
   }
   if (ag == GYRO)
   {
      lsmCsgLow();
      SPIGBUF = addr;                  // no auto increment, R/W bit is zero
      SPIGBUF = data;                  // data
      for(delay=0;delay<28;delay++);   // Wait for transmit to finish
      lsmCsgHigh();
      delay = SPIGBUF;                 // dummy read
      delay = SPIGBUF;                 // dummy read
      return;
   }
}

static char lsmReadRegister(char ag, unsigned char addr)
{
   int delay;

   if (ag == ACCEL)
   {
      lsmCsaLow();
      SPIGBUF = addr | 0x80;           // no auto increment, R/W bit is one
      SPIGBUF = 0x00;                  // data
      SPIABUF = 0x00;                  // needed to have module read
      SPIABUF = 0x00;                  // needed to have module read
      for(delay=0;delay<28;delay++);   // Wait for transmit to finish
      lsmCsaHigh();
      delay = SPIABUF;                 // dummy read
      delay = SPIABUF;                 // actual data read
      delay = SPIGBUF;                 // dummy read
      delay = SPIGBUF;                 // dummy read
      return delay;
   }
   if (ag == GYRO)
   {
      lsmCsgLow();
      SPIGBUF = addr | 0x80;           // no auto increment, R/W bit is one
      SPIGBUF = 0x00;                  // dummy write
      for(delay=0;delay<28;delay++);   // Wait for transmit to finish
      lsmCsgHigh();
      delay = SPIGBUF;                 // dummy read
      delay = SPIGBUF;                 // actual data read
      return delay;
   }
   return 0;
}

void lsmInit(void)
{
   uint16_t i;

   SPIGCON1bits.MODE16 = 0;   // 8-bit mode
   SPIGCON1bits.SMP = 1;      // Sample data at end of data output time
   SPIGCON1bits.CKE = 0;      // SDO changes on idle to active
   SPIGCON1bits.CKP = 1;      // Clock idles high
   SPIGCON1bits.MSTEN = 1;    // Master mode
   SPIGCON1bits.SPRE = 5;     // Secondary prescale bits (based on 140 Mhz)
   SPIGCON1bits.PPRE = 2;     // Primary prescale bits
   SPIGCON2bits.SPIBEN = 1;   // enhanced buffer mode (8 deep fifo)

   SPIACON1bits.MODE16 = 0;   // 8-bit mode
   SPIACON1bits.SMP = 1;      // Sample data at end of data output time
   SPIACON1bits.CKE = 0;      // SDO changes on idle to active
   SPIACON1bits.CKP = 1;      // Clock idles high
   SPIACON1bits.MSTEN = 1;    // Master mode
   SPIACON1bits.SPRE = 5;     // Secondary prescale bits (based on 140 Mhz)
   SPIACON1bits.PPRE = 2;     // Primary prescale bits
   SPIACON2bits.SPIBEN = 1;   // enhanced buffer mode (8 deep fifo)
   SPIACON1bits.DISSCK = 1;   // Disable CLK output (provided by SPI1)
   SPIACON1bits.DISSDO = 1;   // Disable data (MOSI) output (provided by SPI1)

   SPIGSTATbits.SPIEN = 1;    // Enable the module
   SPIASTATbits.SPIEN = 1;    // Enable the module

   lsmCsaHigh();
   lsmCsgHigh();

   for(i=0;i<100;i++);        // a short delay

   //printf("WHOAMI: %X\n\r",0xFF & lsmReadRegister(GYRO,WHO_AM_I_G));

   // ACCEL: 100Hz ODR, Normal power, Z, Y, X Enabled
   lsmWriteRegister(ACCEL, CTRL_REG1_A, 0b01010111);
   // ACCEL: Data LSb @ lower address, +/-2G range, high resolution
   lsmWriteRegister(ACCEL, CTRL_REG4_A, 0b00001000);

   // GYRO: ODR=380, Cut-off=100Hz, Normal power, Z, Y, X Enabled
   lsmWriteRegister(GYRO, CTRL_REG1_G, 0b10111111);

   // Register variables with mmri
   mmriInitVar(20, UINT8, RO, VOL, NOPW, &lsm_data.counter);
   mmriInitVar(21, INT16, RO, VOL, NOPW, &lsm_data.ax);
   mmriInitVar(22, INT16, RO, VOL, NOPW, &lsm_data.ay);
   mmriInitVar(23, INT16, RO, VOL, NOPW, &lsm_data.az);
   mmriInitVar(24, INT16, RO, VOL, NOPW, &lsm_data.gx);
   mmriInitVar(25, INT16, RO, VOL, NOPW, &lsm_data.gy);
   mmriInitVar(26, INT16, RO, VOL, NOPW, &lsm_data.gz);
}

void lsmReadMotionData(void)
{
   static uint16_t count;
   static uint8_t sample_count = 0;
   static LSM_A_RAW a_raw;
   static LSM_G_RAW g_raw;

   lsmCsaLow();
   lsmCsgLow();

   SPIGBUF = 0xE7;         // Read, Auto address increment
   SPIABUF = 0x00;         // dummy write

   for(count=0;count<7;count++)
   {
      SPIGBUF = 0x00;      // dummy write
      SPIABUF = 0x00;      // dummy write
   }

   // Wait for TX/RX to complete.. At this speed (~15us) it really isn't worth
   // going back to do something else and return when an interrupt occurs
   for(count=0;count<100;count++);

   lsmCsaHigh();
   lsmCsgHigh();

   count = SPIGBUF;        // dummy read (the initial address byte)
   count = SPIABUF;        // dummy read

   // Both SPIXBUF registers should now be filled with 7 data bytes
   for(count=0;count<7;count++)
   {
      g_raw.binary_data[count] = SPIGBUF;
      a_raw.binary_data[count] = SPIABUF;
   }

   // Convert binary data to counts
   lsm_data.ax_cnts = (int16_t)((a_raw.out_x_h_a << 8) | a_raw.out_x_l_a);
   lsm_data.ay_cnts = (int16_t)((a_raw.out_y_h_a << 8) | a_raw.out_y_l_a);
   lsm_data.az_cnts = (int16_t)((a_raw.out_z_h_a << 8) | a_raw.out_z_l_a);
   lsm_data.gx_cnts = (int16_t)((g_raw.out_x_h_g << 8) | g_raw.out_x_l_g);
   lsm_data.gy_cnts = (int16_t)((g_raw.out_y_h_g << 8) | g_raw.out_y_l_g);
   lsm_data.gz_cnts = (int16_t)((g_raw.out_z_h_g << 8) | g_raw.out_z_l_g);

   // Convert counts to real units
   // Accelerations in 100ths of m/s^2
   // Angular rates in 10ths of deg/s
   lsm_data.ax = (int16_t)((100*((int32_t)lsm_data.ax_cnts))/1670);
   lsm_data.ay = (int16_t)((100*((int32_t)lsm_data.ay_cnts))/1670);
   lsm_data.az = (int16_t)((100*((int32_t)lsm_data.az_cnts))/1670);
   lsm_data.gx = (int16_t)((10000*((int32_t)lsm_data.gx_cnts))/65536);
   lsm_data.gy = (int16_t)((10000*((int32_t)lsm_data.gy_cnts))/65536);
   lsm_data.gz = (int16_t)((10000*((int32_t)lsm_data.gz_cnts))/65536);
   lsm_data.counter = sample_count;

   sample_count++;
}