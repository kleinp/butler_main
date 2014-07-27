/* uart.h
 * Peter Klein
 * Created on March 19, 2013, 8:06 PM
 * Description:
 *
 * Contains function prototypes and bit masks for setting up the UART(s)
 */

#ifndef UARTH
#define UARTH

// DO NOT CHANGE! Circular buffer depends on overflowing of uint_8
#define UART_BUFFER_SIZE		256

#define	TWO_STOP_BITS			0x01	
#define	ONE_STOP_BIT			0x00
#define	ODD_PARITY			0x02
#define	EVEN_PARITY			0x01
#define	NO_PARITY			0x00

#define	U1				0x01
#define U2				0x02

#define READ                            0
#define WRITE                           1

// Uncomment to enable UART2 (requires extra RAM/ROM)
//#define USING_UART2                     1

typedef struct
{
    uint8_t start;      // index of oldest element
    uint8_t end;        // index for new elements
    uint8_t lastop;     // was last operation READ=0, WRITE=1
    uint8_t buffer[UART_BUFFER_SIZE];
} circBuf;

// Simple circular buffer implementation as a software buffer
uint8_t cbFull(circBuf *cb);
uint8_t cbEmpty(circBuf *cb);
void cbInit(circBuf *cb);
void cbWrite(circBuf *cb, uint8_t data);
uint8_t cbRead(circBuf *cb);

void u1Init(uint32_t baud_rate, int8_t parity_mode, int8_t stop_bits);

#ifdef USING_UART2
void u2Init(uint32_t baud_rate, int8_t parity_mode, int8_t stop_bits);
#endif

void uChangeBaud(uint32_t baud_rate, int8_t where);
void uPutChar(int8_t c, int8_t where);
int8_t uGetChar(int8_t where);
int8_t uCharAvailable(int8_t where);
void uFlush(int8_t where);
// TX/RX Interrupt routines
// Write override for printf
uint8_t getString(int8_t *buf, uint8_t max_len, uint8_t from, uint8_t to);

#endif
