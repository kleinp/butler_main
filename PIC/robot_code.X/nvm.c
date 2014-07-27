/* 
 * File:   nvm.c
 * Author: Peter
 *
 * Created on January 1, 2014, 5:12 PM
 */

#include "system.h"
#include "nvm.h"

// Generate start
void i2cStart(void)
{
   I2C1CONbits.SEN = 1;
   while (I2C1CONbits.SEN);
}

// Generate restart
void i2cRestart(void)
{
   I2C1CONbits.RSEN = 1;
   while (I2C1CONbits.RSEN);
}

// Generate stop
void i2cStop(void)
{
   I2C1CONbits.PEN = 1;
   while (I2C1CONbits.PEN);
}

// Write "data" out
void i2cWrite(uint8_t data)
{
   I2C1TRN = data;
   while (I2C1STATbits.TBF);
}

// Wait for module to become idle
void i2cIdle(void)
{
   while(I2C1STATbits.TRSTAT);
}

// Return ACK status
uint8_t i2cAckStat(void)
{
   return (I2C1STATbits.ACKSTAT);
}

// Send out NACK
void i2cNACK(void)
{
   I2C1CONbits.ACKDT = 1;     // NACK instead of ACK
   I2C1CONbits.ACKEN = 1;     // Send
   while (I2C1CONbits.ACKEN); // wait for complete
}

// Send out ACK
void i2cACK(void)
{
   I2C1CONbits.ACKDT = 0;     // ACK instead of NACK
   I2C1CONbits.ACKEN = 1;     // Send
   while (I2C1CONbits.ACKEN); // wait for complete
}

// Read a byte from module
uint8_t i2cRead(void)
{
   I2C1CONbits.RCEN = 1;      // enable receive
   Nop();
   while(!I2C1STATbits.RBF);  // wait for receive buffer to fill
   return(I2C1RCV);
}

// Check write complete status of EEPROM
// It will not respond until it is done writing, so keep checking to see if
// it is done
uint8_t nvmAckPoll(void)
{
   i2cIdle();                 // wait for idle
   i2cStart();                // generate start

   if (I2C1STATbits.BCL)
      return(-1);             // bus collision, return

   else
   {
      i2cWrite(0xA0);         // write control byte (R/nW = 0)
      i2cIdle();              // wait for idle

      if (I2C1STATbits.BCL)
         return(-1);          // bus collision, return

      return(i2cAckStat());   // 0 if ACK was received
   }
}

// Initialized the I2C module of the microcontroller
void nvmInit(void)
{
   I2C1BRG = (uint16_t)((getClockFreq()/843882)-1);

   I2C1CON = 0;               // clear the module configuration register
   I2C1CONbits.DISSLW = 0;    // enable slew rate control to operate at 400kHz
   I2C1CONbits.SCLREL = 1;    // release clock line

   I2C1RCV = 0;               // clear the receive register
   I2C1TRN = 0;               // clear the transmit register

   I2C1CONbits.I2CEN = 1;     // enable the I2C module
}

// Write a single byte to an address
void nvmWriteByte(uint8_t addr, uint8_t data)
{
   i2cIdle();                 // wait for module to be idle
   i2cStart();                // generate start
   i2cWrite(0xA0);            // control byte (R/nW = 0)

   i2cIdle();                 // wait for module to be idle
   i2cWrite(addr);            // write address

   i2cIdle();                 // wait for idle
   i2cWrite(data);            // write data

   i2cIdle();                 // wait for idle
   i2cStop();                 // generate stop

   while(nvmAckPoll());         // poll until ACK is received (done writing)

   i2cStop();                 // generate stop .. polling generates starts
   i2cIdle();                 // wait for idle
}

// Read a single byte from address
uint8_t nvmReadByte(uint8_t addr)
{
   uint8_t data;

   i2cIdle();                 // wait for idle
   i2cStart();                // generate start
   i2cWrite(0xA0);            // control byte (R/nW=0)
   i2cIdle();                 // wait for idle
   i2cWrite(addr);            // write address we want to read
   i2cIdle();                 // wait for idle

   i2cRestart();              // generate restart
   i2cWrite(0xA1);            // control byte (R/nW=1)
   i2cIdle();                 // wait for idle

   data = i2cRead();          // read data byte
   i2cNACK();                 // send NACK
   i2cStop();                 // generate stop

   return(data);
}

// Write *length* bytes starting at *addr* from data pointer. Uses page write
// capability of EEPROM to decrease write time
void nvmWriteArray(uint8_t addr, uint16_t length, uint8_t *data)
{
   uint8_t page_remain;

   while(length)
   {
      i2cIdle();                 // wait for idle
      i2cStart();                // generate start
      i2cWrite(0xA0);            // control byte (R/nW = 0)
      i2cIdle();                 // wait for module to be idle
      i2cWrite(addr);            // write address
      i2cIdle();                 // wait for idle
      page_remain = 8-(addr%8);  // figure out how many bytes are left
                                 // to write in the current page

      while(length)              // as long as there is data left to write
      {
         i2cWrite(*data++);      // write data
         i2cIdle();              // wait for idle
         length--;               // decrement how many bytes left to write
         page_remain--;          // decrement bytes remaining in page
         addr++;                 // keep track of address

         if (page_remain == 0 || length == 0)   // page boundary reached
         {
            i2cStop();           // generate stop
            i2cIdle();           // wait for idle
            while(nvmAckPoll()); // poll until ACK is received (done writing)
            i2cStop();           // generate stop .. polling generates starts
            i2cIdle();           // wait for idle
            break;               // send next page address (or stop)
         }
      }
   }
}

// Read *length* bytes starting at *addr* and save to pointed space
void nvmReadArray(uint8_t addr, uint16_t length, uint8_t *data)
{
   i2cIdle();                 // wait for idle
   i2cStart();                // generate start
   i2cWrite(0xA0);            // control byte (R/nW=0)
   i2cIdle();                 // wait for idle
   i2cWrite(addr);            // write address we want to read
   i2cIdle();                 // wait for idle

   i2cRestart();              // generate restart
   i2cWrite(0xA1);            // control byte (R/nW=1)

   while(length--)
   {
      i2cIdle();              // wait for idle
      *data++ = i2cRead();    // put data in buffer

      if(length)
         i2cACK();            // if there is still data to come, generate ACK
      else
         i2cNACK();           // if this is the last byte, generate NACK
   }

   i2cStop();                 // generate stop
}
