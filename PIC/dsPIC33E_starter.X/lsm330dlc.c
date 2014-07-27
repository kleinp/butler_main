/* lsm330dlc.c
 * Peter Klein
 * Created on April 28, 2013, 7:42 PM
 * Description:
 *
 */

#include "system.h"
#include "lsm330dlc.h"

#include "basicIO.h"
#include "uart.h"

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
   unsigned int i;

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

   printF("WHOAMI: %Xuc\n\r",lsmReadRegister(GYRO,WHO_AM_I_G));

   // ACCEL: 100Hz ODR, Normal power, Z, Y, X Enabled
   lsmWriteRegister(ACCEL, CTRL_REG1_A, 0b01010111);
   // ACCEL: Data LSb @ lower address, +/-2G range, high resolution
   lsmWriteRegister(ACCEL, CTRL_REG4_A, 0b00001000);

   // GYRO: ODR=380, Cut-off=100Hz, Normal power, Z, Y, X Enabled
   lsmWriteRegister(GYRO, CTRL_REG1_G, 0b10111111);

}

void lsmReadMotionData(void)
{
   int count;
   int tmp_a, tmp_g;
   double val_a, val_g;
   unsigned char rcv_g[7];
   unsigned char rcv_a[7];

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
      rcv_g[count] = SPIGBUF;
      rcv_a[count] = SPIABUF;

      //printF("%02Xuc,%02Xuc,",rcv_g[count], rcv_a[count]);
   }
   //printF("\n\r");

   for(count=1;count<7;count+=2)
   {
      tmp_g = (int)((rcv_g[count+1] << 8) | rcv_g[count]);
      tmp_a = (int)((rcv_a[count+1] << 8) | rcv_a[count]);

      val_g = ((double)tmp_g)/131.072;
      val_a = ((double)tmp_a)/16384.0;

      printF("%0+2.2d,%0+2.2d,", val_g, val_a);
   }
   printF("\n\r");

}
