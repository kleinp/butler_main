/* messaging.c
 * Peter Klein
 * Created on June 28, 2013, 8:43 PM
 * Description:
 *
 */

#include "system.h"
#include "messaging.h"

#include "basicIO.h"
#include "uart.h"

/* GLOBAL VARIABLES ***********************************************************/
FRAME_BUFFER rx_buffer[NUM_RX_BUFFER];
FRAME_BUFFER tx_buffer[NUM_TX_BUFFER];
FRAME_HEADER sys_messages;

int8_t *rx_binary_data_ptr;
int16_t rcv_count=0;
int16_t msg_length=0;
int8_t source_dest=0;

uint8_t msg_count=0; // 4-bit roll-over counter for each message
// sent out.

uint8_t packet_lengths[100]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 17, 3, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/******************************************************************************/

/*******************************************************************************
 * Function:      crcInitEngine
 * Inputs:        None
 * Outputs:       None
 * Description:   Initializes the hardware CRC calculator to the correct data
 *                width and polynomial for 16-bit CRC CCITT
 * ****************************************************************************/
uint8_t *ptr=(uint8_t *)&CRCDATL;

void crcInitEngine(void)
{
   CRCCON1bits.CRCEN=1; // enable the module
   CRCCON2bits.PLEN=15; // polynomial length - 1
   CRCXORL=0x1021; // 16-bit CRC CCITT
   CRCXORH=0x0000; // N/A for 16-bit polynomial

   CRCCON2bits.DWIDTH=7; // data width - 1
   CRCCON1bits.LENDIAN=0; // shift data in Msb first
   CRCCON1bits.CRCISEL=0; // interrupt on shift complete (no using interrupts)
}

/*******************************************************************************
 * Function:      calculateCRC
 * Inputs:        <char *data> pointer to character sized data
 *                <int length> how many bytes to run the algorithm on
 * Outputs:       <unsigned int> resulting crc
 * Description:   This function takes a pointer to data and data length (some
 *                data may be '0') and returns the 16-bit CCITT CRC value. This
 *                function simply calls the lower level functions in one go
 * ****************************************************************************/
uint16_t calculateCRC(int8_t *data, int16_t length)
{
   crcInitMsg();
   crcAddMsg(data, length);
   return crcCalculateMsg();
}

/*******************************************************************************
 * Function:      crcInitMsg
 * Inputs:        None
 * Outputs:       None
 * Description:   Initializes the hardware CRC engine for a new data set.
 *                official CCITT CRC starts with 0xFFFF loaded.
 * ****************************************************************************/
void crcInitMsg(void)
{
   CRCWDATL=0; // clear old result
   CRCWDATH=0;

   *ptr=0xFF; // initial value of 0xFFFF
   *ptr=0xFF; // for "official" 16-bit CRC-CCITT

   CRCCON1bits.CRCGO=1; // start the module
}

/*******************************************************************************
 * Function:      crcAddMsg
 * Inputs:        <char *data> pointer to character sized data
 *                <int length> how many bytes to run the algorithm on
 * Outputs:       None
 * Description:   This function takes a pointer to data and data length (some
 *                data may be '0') and computes the CRC of it. This function
 *                may be called as many times as desired between crcInitMsg()
 *                and crcCalculateMsg() to build any sized message.
 * ****************************************************************************/
void crcAddMsg(int8_t *data, int16_t length)
{
   int16_t i;

   for(i=0; i<length; i++)
   { // it only takes 1/2 instruction cycle to calculate
      *ptr= *data++; // the next value, so don't worry about
   } // filling up the FIFO
}

/*******************************************************************************
 * Function:      crcAddByte
 * Inputs:        <char data> single byte to add
 * Outputs:       None
 * Description:   This function adds a single byte to the CRC "stack"
 * ****************************************************************************/
void crcAddByte(int8_t data)
{
   *ptr=data;
}

/*******************************************************************************
 * Function:      crcCalculateMsg
 * Inputs:        None
 * Outputs:       <unsigned int> resulting crc
 * Description:   This function finalizes the crc based on crcInitMsg and
 *                crcAddMsg by "pushing" out the result
 * ****************************************************************************/
uint16_t crcCalculateMsg(void)
{
   *ptr=0x00; // necessary to "push" out the CRC
   *ptr=0x00;

   while (IFS4bits.CRCIF!=1); // Wait for module to complete the calculation
   CRCCON1bits.CRCGO=0; // Stop the module
   Nop(); // necessary to get the correct value

   return CRCWDATL;
}

/*******************************************************************************
 * Function:      msgSend
 * Inputs:        <int bnum> which buffer the data should be sent from
 *                <char port> which UART the data should be sent out of
 * Outputs:       None
 * Description:   This function sends out the message in <bnum>, complete with
 *                start of frame, auto-indexing message count and CRC at the end
 * ****************************************************************************/
void msgSend(int16_t bnum, int8_t port)
{
   int16_t packet_length=(int16_t)packet_lengths[tx_buffer[bnum].header.type]+2;
   int8_t *binary_data_ptr= &tx_buffer[bnum].binary_data[0];
   uint16_t i, crc;

   // Put the message count into the header and increment for next message
   tx_buffer[bnum].header.count=msg_count;
   msg_count++;
   if (msg_count>15)
      msg_count=0;

   crcInitMsg();
   crcAddByte('[');
   crcAddMsg(binary_data_ptr, packet_length);
   crc=crcCalculateMsg();

   binary_data_ptr= &tx_buffer[bnum].binary_data[0];

   uPutChar('[', port);
   for(i=0; i<packet_length; i++)
      uPutChar(*binary_data_ptr++, port);

   uPutChar((crc>>8) & 0xFF, port);
   uPutChar(crc&0xFF, port);
}

/*******************************************************************************
 * Function:      msgSendSys
 * Inputs:        <unsigned char count_val> The message count to use when
 *                sending this message. Can be "AUTO_COUNT" to use the system
 *                counter, but for ACK/NACK messages you will want to use the
 *                received count instead
 *                <char port> which UART to use
 *                <char type> which type of system message to send
 * Outputs:       None
 * Description:   This function will send a system message 0x01-0x05 (the ones
 *                the don't have any associated length or data bytes)
 * ****************************************************************************/
void msgSendSys(uint8_t count_val, int8_t port, int8_t type)
{
   uint16_t crc;

   port=U1;

   crcInitMsg();
   crcAddByte('[');

   if (count_val==AUTO_COUNT)
   {
      count_val=msg_count;
      msg_count++;
      if (msg_count>15)
         msg_count=0;
   }

   sys_messages.count=count_val;
   sys_messages.sd=source_dest;
   sys_messages.ack=FALSE;

   crcAddByte(sys_messages.cntrl);
   crcAddByte(type);
   crc=crcCalculateMsg();

   uPutChar('[', port);
   uPutChar(sys_messages.cntrl, port);
   uPutChar(type, port);
   uPutChar((crc>>8) & 0xFF, port);
   uPutChar(crc&0xFF, port);
}

/*******************************************************************************
 * Function:      msgReceive
 * Inputs:        <int bnum> which buffer the data should be put into
 *                <char port> which UART to use
 * Outputs:       None
 * Description:   This function attempts to capture a message from the <port>
 *                specified. It will discard characters until the SOF is
 *                received, and then store the control, type, and length bytes
 *                and all following data. Once the number of bytes specified by
 *                length have been received, it calls the msgParse() function.
 *
 *                NOTE: IMPLEMENT A TIMEOUT TO SEND A NACK
 * ****************************************************************************/
void msgReceive(int16_t bnum, int8_t port)
{
   int8_t c;

   while (uCharAvailable(port))
   {
      c=uGetChar(port);

      switch (rcv_count)
      {
            // Initially, one wants to skip any bytes other than a start of frame
            // (SOF) character. Since data is binary (or possibly ASCII), the SOF
            // character may be encountered when not intended, in which case an
            // error will eventually occur when the CRC value does not match at the
            // end of the message
         case 0:
            if (c=='[')
            {
               rx_binary_data_ptr= &rx_buffer[bnum].binary_data[0];
               rcv_count++;
            }
            break;
            // The next character after SOF is the control byte. It contains:
            // - Does the receiver need to send an ACK message
            // - The source/desination bit
            // - 2 bits that may be used in the future
            // - A 4-bit message count to keep track of order and ACKing
         case 1:
            *rx_binary_data_ptr++ =c;
            rcv_count++;

            // If source matches destination, discard the message
            if (rx_buffer[bnum].header.sd==source_dest)
               rcv_count=0;

            break;
            // An 8-bit value for message type. The message type has to be defined
            // in the messaging.h file by the user. Message lengths are fixed and
            // defined in packet_lengths[] in messaging.c
            // NOTE: message type 0x00 does not exist, 0x01-0x0F are reserved
            //       "system" messages
         case 2:
            *rx_binary_data_ptr++ =c;
            rcv_count++;

            // packet_length is defined, but does not take into account:
            // SOF, CTRL, TYPE, ... CRC1, CRC2
            msg_length=(int16_t)packet_lengths[(int16_t)c]+4;

            if (c<0x06) // message type 0x01-0x05 are complete
            {
               msgParse(bnum, port);
               rcv_count=0;
            }
            break;
            // This case handles the rest of the data and CRC. It will collect
            // as many more characters are specified by the 'length' byte above
            // and then attempt to parse the message.
            // IMPLEMENT A TIMEOUT HERE!!!
         default:
            if (rcv_count<msg_length)
            {
               *rx_binary_data_ptr++ =c;
               rcv_count++;
            }
            else
            {
               *rx_binary_data_ptr=c;
               msgParse(bnum, port);
               rcv_count=0;
            }
            break;
      }
   }
}

/*******************************************************************************
 * Function:      msgParse
 * Inputs:        <int bnum> which buffer the data is from
 * Outputs:       None
 * Description:   This function will first check the CRC, send an ACK
 *                (if requested) and the call the appropriate function for the
 *                message type.
 *
 *                NOTE: doesn't work for system messages right now 0x01-0x05.
 *                      the CRC won't match and this function will simply return
 * ****************************************************************************/
void msgParse(int16_t bnum, int8_t port)
{
   int16_t packet_length=(int16_t)packet_lengths[tx_buffer[bnum].header.type]+2;
   int8_t *binary_data_ptr= &rx_buffer[bnum].binary_data[0];
   uint16_t crc_calc, crc_received;

   // Re-calculate the CRC
   crcInitMsg();
   crcAddByte('[');
   crcAddMsg(binary_data_ptr, packet_length);
   crc_calc=crcCalculateMsg();

   // Get the CRC from the packet
   binary_data_ptr= &rx_buffer[bnum].binary_data[packet_length];
   crc_received=(uint16_t)*binary_data_ptr++;
   crc_received<<=8;
   crc_received|=(uint16_t)*binary_data_ptr&0xFF;

   // If CRC don't match, send NACK if an ACK was requested, and then return
   // without parsing the rest of the message into variables
   if (crc_calc!=crc_received)
   {
      if (rx_buffer[bnum].header.ack)
         msgSendSys(rx_buffer[bnum].header.count, port, NACK);
      return;
   }

   // send ACK if requested
   if (rx_buffer[bnum].header.ack)
      msgSendSys(rx_buffer[bnum].header.count, port, ACK);

   switch (rx_buffer[bnum].header.type)
   {
      case 16: msgParse_16(bnum);
         break;
      case 17: msgParse_17(bnum);
         break;
      default: break;
   }
}

/*******************************************************************************
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * THE BELOW FUNCTIONS ARE PARTIALLY AUTO GENERATED AND SPECIFIC TO THE
 * PROJECT. THEY DEFINE HOW THE USER MESSAGE TYPES (0x10-0xFF) SHOULD BE
 * BUILT AND PARSED...
 *
 * NOTE1- THEY ARE NOT ACTUALLY AUTO-GENERATED RIGHT NOW!!
 * NOTE2- The buffers are 'global' variables, so these function may be more
 *        suitable in a different function where other
 *        relevant variables are defined
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * ****************************************************************************/

/*******************************************************************************
 * Function:      msgBuild_16
 * Inputs:        <int bnum> which buffer the data should go into
 * Outputs:       None
 * Description:   This function fills the buffer <bnum> with the data in
 *                message type 16, as defined by PACKET_16 in messaging.h
 *
 *                ** THIS FUNCTION NEEDS TO BE AUTO-GENERATED ALONG WITH
 *                   NECESSARY HEADER FILES **
 * ****************************************************************************/
void msgBuild_16(int16_t bnum)
{
   // Header setup information (static)
   tx_buffer[bnum].header.ack=FALSE;
   tx_buffer[bnum].header.sd=source_dest;
   tx_buffer[bnum].header.type=16;

   // Data to be filled in
   tx_buffer[bnum].data_16.A1=5;
   tx_buffer[bnum].data_16.T1=12.5;
   tx_buffer[bnum].data_16.T2=15;
   tx_buffer[bnum].data_16.T3=40052.94;
   tx_buffer[bnum].data_16.T4= -3.14159;
}

void msgParse_16(int16_t bnum)
{
   return;
}

uint8_t msg17_count=0;

void msgBuild_17(int16_t bnum)
{
   tx_buffer[bnum].header.ack=FALSE;
   tx_buffer[bnum].header.sd=source_dest;
   tx_buffer[bnum].header.f1=0;
   tx_buffer[bnum].header.f2=0;
   tx_buffer[bnum].header.type=17;

   tx_buffer[bnum].data_17.B1=0xAA;
   tx_buffer[bnum].data_17.C1=msg17_count++;
   tx_buffer[bnum].data_17.D1=0xBB;
}

void msgParse_17(int16_t bnum)
{
   int16_t i;

   for(i=0; i<10; i++)
      uPutChar(rx_buffer[bnum].binary_data[i], U1);
   return;
}