/* uart.h
 * Peter Klein
 * Created on March 19, 2013, 8:06 PM
 * Description:
 *
 * Contains functions for initializing, sending, and receiving characters
 * through the UART. Uses built in 4-byte buffer, but also expands to a much
 * larger ring buffer. Contains putChar and getChar functions and write function
 * override to allow usage of printf.
 */

#include "system.h"
#include "uart.h"

// UART buffers
circBuf rx_buffer_u1;
circBuf tx_buffer_u1;

#ifdef USING_UART2
circBuf rx_buffer_u2;
circBuf tx_buffer_u2;
#endif

/*******************************************************************************
 * Function:      cbFull
 * Inputs:        <circBuf *> pointer of buffer to check
 * Outputs:       <uint8> state of buffer
 * Description:   Checks to see if specified buffer is full and returns
 *                1 if buffer is full
 *                0 if buffer is not full
 * ****************************************************************************/
uint8_t cbFull(circBuf *cb)
{
   if (cb->lastop == READ)      // last operation was a read
      return(0);                 // thefore the buffer can't be full
   else
   {
      if (cb->start == cb->end)
         return(1);              // yes, the buffer is full
   }
   return(0);
}

/*******************************************************************************
 * Function:      cbEmpty
 * Inputs:        <circBuf *> pointer of buffer to check
 * Outputs:       <uint8> state of buffer
 * Description:   Checks to see if specified buffer is empty and returns
 *                1 if buffer is empty
 *                0 if buffer is not empty
 * ****************************************************************************/
uint8_t cbEmpty(circBuf *cb)
{
   if (cb->lastop == WRITE)     // last operationw as a write
      return(0);                 // therefore, the buffer can't be empty
   else
   {
      if (cb->start == cb->end)
         return(1);              // yes, the buffer is empty
   }
   return(0);
}

/*******************************************************************************
 * Function:      cbInit
 * Inputs:        <circBuf *> pointer of buffer
 * Outputs:       None
 * Description:   Initializes the start/end and last operation variables of the
 *                circular buffer used. Can also be used to 'clear' the buffer
 * ****************************************************************************/
void cbInit(circBuf *cb)
{
   cb->lastop = READ;
   cb->end = 0;
   cb->start = 0;
}

/*******************************************************************************
 * Function:      cbWrite
 * Inputs:        <circBuf *> pointer of buffer
 *                <uint8> data
 * Outputs:       None
 * Description:   Places the data byte into the buffer. Will overwrite buffer
 *                locations, so check to make sure there is room in the buffer
 *                beforehand
 * ****************************************************************************/
void cbWrite(circBuf *cb, uint8_t data)
{
   cb->lastop = WRITE;
   cb->buffer[cb->end++] = data;
}

/*******************************************************************************
 * Function:      cbRead
 * Inputs:        <circBuf *> pointer of buffer to read from
 * Outputs:       <uint8> data
 * Description:   Returns a value from the circular buffer. Does not perform
 *                any checks, so make sure there is a value to be read
 * ****************************************************************************/
uint8_t cbRead(circBuf *cb)
{
   cb->lastop = READ;
   return(cb->buffer[cb->start++]);
}

/*******************************************************************************
 * Function:      u1Init
 * Inputs:        <uint32 baud_rate> desired baud rate of UART
 *                <int8 parity_mode> ODD_PARITY/EVEN_PARITY/NO_PARITY
 *                <int9 stop_bits> ONE_STOP_BIT/TWO_STOP_BITS
 * Outputs:       None
 * Description:   This function initializes the UART registers, extended FIFO
 *                buffer, and interrupts for complete putChar and getChar
 *                functionality.
 * ****************************************************************************/
void u1Init(uint32_t baud_rate, int8_t parity_mode, int8_t stop_bits)
{
   cbInit(&rx_buffer_u1);
   cbInit(&tx_buffer_u1);
	
	U1MODE = 0;		// reset UART
	
	// 1 or 2 stop bits
	U1MODEbits.STSEL = stop_bits;
	
	// Parity type
	U1MODEbits.PDSEL = parity_mode;

   // Set baud rate
   U1MODEbits.BRGH = 1;
   uChangeBaud(baud_rate, U1);
	
	// Set up interrupts
	U1STAbits.UTXISEL1 = 1;		// interrupt when the last byte goes to be sent out
	U1STAbits.UTXISEL0 = 0;
	U1STAbits.URXISEL1 = 0;		// interrupt when there is anything in RX buffer
	U1STAbits.URXISEL0 = 0;	
	
	IEC0bits.U1RXIE = 1;			// enable vectored interrupt for tx and rx
	IEC0bits.U1TXIE = 1;
	
	IPC2bits.U1RXIP = 1;       // set interrupt priority to 1 (pretty low)
	IPC3bits.U1TXIP = 1;
	
	IFS0bits.U1RXIF = 0;       // clear interrupt flags
   IFS0bits.U1TXIF = 0;

   // Enable UART
	U1MODEbits.UARTEN = 1;
	U1STAbits.UTXEN = 1;			// enable tx
}	

/*******************************************************************************
 * Function:      u2Init
 * Inputs:        <uint32 baud_rate> desired baud rate of UART
 *                <int8 parity_mode> ODD_PARITY/EVEN_PARITY/NO_PARITY
 *                <int9 stop_bits> ONE_STOP_BIT/TWO_STOP_BITS
 * Outputs:       None
 * Description:   This function initializes the UART registers, extended FIFO
 *                buffer, and interrupts for complete putChar and getChar
 *                functionality.
 * ****************************************************************************/
#ifdef USING_UART2
void u2Init(uint32_t baud_rate, int8_t parity_mode, int8_t stop_bits)
{
	cbInit(&rx_buffer_u2);
   cbInit(&tx_buffer_u2);

	U2MODE = 0;		// reset UART

	// 1 or 2 stop bits
	U2MODEbits.STSEL = stop_bits;

	// Parity type
	U2MODEbits.PDSEL = parity_mode;

   // Set baud rate
   U2MODEbits.BRGH = 1;
   uChangeBaud(baud_rate, U2);

	// Set up interrupts
	U2STAbits.UTXISEL1 = 1;		// interrupt when the last byte goes to be sent out
	U2STAbits.UTXISEL0 = 0;
	U2STAbits.URXISEL1 = 0;		// interrupt when there is anything in RX buffer
	U2STAbits.URXISEL0 = 0;

	IEC1bits.U2RXIE = 1;			// enable vectored interrupt for tx and rx
	IEC1bits.U2TXIE = 1;

	IPC7bits.U2RXIP = 1;       // set interrupt priority to 1 (pretty low)
	IPC7bits.U2TXIP = 1;

	IFS1bits.U2RXIF = 0;       // clear interrupt flags
   IFS1bits.U2TXIF = 0;

   // Enable UART
	U2MODEbits.UARTEN = 1;
	U2STAbits.UTXEN = 1;			// enable tx
}
#endif

/*******************************************************************************
 * Function:      uChangeBaud
 * Inputs:        <uint32_t baud_rate> desired baud rate of UART
 *                <int8_t where> which UART should be affected
 * Outputs:       None
 * Description:   This function updates the UxBRG based on the desired baud
 *                rate given the current processor fequency
 * ****************************************************************************/
void uChangeBaud(uint32_t baud_rate, int8_t where)
{
   uint32_t clock_freq = getClockFreq();
   
   switch(where)
   {
      case U1:
         U1BRG = (unsigned int)(clock_freq/(8*baud_rate)-1);
         break;
      case U2:
         U2BRG = (unsigned int)(clock_freq/(8*baud_rate)-1);
         break;
      default:
         break;
   }
}

/*******************************************************************************
 * Function:      uPutChar
 * Inputs:        <int8_t where> which UART should be affected
 * Outputs:       None
 * Description:   Places a character into the software FIFO for transmission.
 *                If not already going, this function starts the TX process by
 *                putting a character into the hardare buffer
 * ****************************************************************************/
void uPutChar(int8_t c, int8_t where)
{
	switch(where)		// which buffer shall the character be put into?
	{
		case U1:
			while(cbFull(&tx_buffer_u1));    // wait for there to be space
                                          // in the software buffer
         cbWrite(&tx_buffer_u1, c);

			if (!U1STAbits.UTXBF)            // Start the tx process
            U1TXREG = cbRead(&tx_buffer_u1);
				
			break;
		case U2:
         #ifdef USING_UART2
			while(cbFull(&tx_buffer_u2));    // wait for there to be space
                                          // in the software buffer
         cbWrite(&tx_buffer_u2, c);

			if (!U1STAbits.UTXBF)            // Start the tx process
            U1TXREG = cbRead(&tx_buffer_u2);

			break;
         #endif
			break;
		default:
			break;
	}
}

/*******************************************************************************
 * Function:      uGetChar
 * Inputs:        <int8_t where> which UART should be affected
 * Outputs:       <int8_t c> returned character
 * Description:   Returns the oldest character in the software receive FIFO
 * ****************************************************************************/
int8_t uGetChar(int8_t where)
{
	switch(where)		// which buffer shall the character be gotten from?
	{
		case U1:
			while(cbEmpty(&rx_buffer_u1));    // wait for a character
			return(cbRead(&rx_buffer_u1));
			break;
		case U2:
         #ifdef USING_UART2
			while(cbEmpty(&rx_buffer_u2));    // wait for a character
			return(cbRead(&rx_buffer_u2));
         #endif
			break;
		default:
			break;
	}
	return(0);
}

/*******************************************************************************
 * Function:      uCharAvailable
 * Inputs:        <int8_t where> which UART should be affected
 * Outputs:       <int8_t available> character available? 1=yes 0=no
 * Description:   This function returns a 1 if there is an available character
 *                in the software receive buffer, and a 0 if there is not.
 *                This function should be used in conjunction with uGetChar()
 *                as it is a blocking function if there are no characters
 *                available to get.
 * ****************************************************************************/
int8_t uCharAvailable(int8_t where)
{
	switch(where)
	{
		case U1:
			return(!cbEmpty(&rx_buffer_u1));
			break;
		case U2:
         #ifdef USING_UART2
			return(!cbEmpty(&rx_buffer_u2));
         #endif
			break;
		default:
			break;
	}
	return(0);
}	

/*******************************************************************************
 * Function:      uFlush
 * Inputs:        <int8_t where> which UART should be affected
 * Outputs:       None
 * Description:   This function flushes input and output software buffers of
 *                the module
 * ****************************************************************************/
void uFlush(int8_t where)
{
	switch(where)
	{
		case U1:
			cbInit(&rx_buffer_u1);
			cbInit(&tx_buffer_u1);
			break;
		case U2:
         #ifdef USING_UART2
			cbInit(&rx_buffer_u2);
			cbInit(&tx_buffer_u2);
         #endif
			break;
		default:
			break;
	}
}

/*******************************************************************************
 * Function:      _U1/2RXInterrupt / _U1/2TXInterrupt
 * Inputs:        None
 * Outputs:       None
 * Description:   These functions are the vectored interrupts of the UART module
 *                For transmit, they more characters from the software FIFO
 *                buffer into the 4-byte deep hardware FIFO. To receive, they
 *                move characters from the 4-byte deep hardware FIFO into the
 *                software buffer. The 4-byte deep hardware FIFO allows these
 *                functions to operate at a lower interrupt priority, since
 *                at a normal 115200 baud, it would take ~0.3 ms to fill the
 *                buffer
 * ****************************************************************************/
void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{
	static int8_t val;
	while(U1STAbits.URXDA)	// there is data in the built-in RX FIFO
	{
		val = U1RXREG;
		cbWrite(&rx_buffer_u1, val);		// move the data to the bigger "software" buffer
	}

	IFS0bits.U1RXIF = 0;
}

void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void)
{
	while(!U1STAbits.UTXBF && !cbEmpty(&tx_buffer_u1))		// there is space in the built-in TX FIFO
	{
		U1TXREG = cbRead(&tx_buffer_u1);
	}
	
	IFS0bits.U1TXIF = 0;
}	

#ifdef USING_UART2
void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void)
{
	static int8_t val;
	while(U2STAbits.URXDA)	// there is data in the built-in RX FIFO
	{
		val = U2RXREG;
		cbWrite(&rx_buffer_u2, val);		// move the data to the bigger "software" buffer
	}

	IFS1bits.U2RXIF = 0;
}

void __attribute__((interrupt, no_auto_psv)) _U2TXInterrupt(void)
{
	while(!U2STAbits.UTXBF && !cbEmpty(&tx_buffer_u2))		// there is space in the built-in TX FIFO
	{
		U2TXREG = cbRead(&tx_buffer_u2);
	}
		
	IFS1bits.U2TXIF = 0;
}
#endif

/*******************************************************************************
 * Function:      write
 * Inputs:        <int handle> where to write the data. Currently only works for uart
 *                <void *buffer> pointer to data
 *                <unsigned int len> length of data to write
 * Outputs:       <int len> number of characters written
 * Description:   This function replaces the built-in write function so that
 *                it can be re-directed to UART1
 * ****************************************************************************/
int __attribute__((__section__(".libc.write"))) write(int16_t handle, void *buffer, uint16_t len)
{
   int16_t i;
   switch (handle)
   {
      default:
      case 0:
      case 1:
      case 2:
         for (i=len;i;i--)
            uPutChar(*(int8_t*)buffer++, U1);
         break;
   }
   return(len);
}

/*******************************************************************************
 * Function:      getString
 * Inputs:        <int8_t *buf> The buffer the string should be stored in
 *                <uint8 max_len> The maximum number of characters to return
 *                <uint8 from> from which UART the characters should come from
 *                <uint8 to> which UART the characters should be echo'd to, or
 *                           use NULL for none.
 * Outputs:       <int len> number of characters returned in buffer, no including
 *                          the null (\0) character
 * Description:   Gets a string of defined length or shorter from a UART buffer.
 *                Echoes the input back to the user real-time, and can handle
 *                backspace!
 * ****************************************************************************/
uint8_t getString(int8_t *buf, uint8_t max_len, uint8_t from, uint8_t to)
{
   int8_t c;
   uint8_t count = 0;

   while(count < max_len-1)
   {
      c = uGetChar(from);

      if (c == '\n' || c == '\r')         // ENTER pressed
         break;
      else if (c >= ' ' && c <= '~')      // ASCII character entered
      {
         uPutChar(c, to);
         *buf++ = c;
         count++;
      }
      else if (c == '\b' && count > 0)    // Backspace (don't allow backup past first char)
      {
         uPutChar('\b', to);
         uPutChar(' ', to); 
         uPutChar('\b', to);
         buf--;
         count--;
      }
   }
   
   *buf = '\0';      // null terminate
   return(count);
}
