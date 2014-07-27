/* messaging.h
 * Peter Klein
 * Created on June 28, 2013, 8:43 PM
 * Description:
 *
 */

#ifndef MESSAGING_H
#define	MESSAGING_H

#include <stdint.h>

#define MAX_PACKET_SIZE             20
#define NUM_TX_BUFFER               1
#define NUM_RX_BUFFER               1
#define NUM_DEFINED_PACKETS         20

#define AUTO_COUNT                  0xFF
#define ACK                         0x01
#define NACK                        0x02

// ** AUTO GENERATED (eventually) **
typedef struct
{
    uint16_t counter;
    int16_t Ax;
    int16_t Ay;
    int16_t Az;
    int16_t Gx;
    int16_t Gy;
    int16_t Gz;
    int16_t angle;
} PACKET_18;

// ** END AUTO GENERATED **

typedef struct
{
    union
    {
        uint8_t cntrl;
        struct
        {
            uint8_t count:4;               // incrementing and roll-over counter
            uint8_t f2:1;                  // future field 2
            uint8_t f1:1;                  // future field 1
            uint8_t sd:1;                  // source/destination. Must be defined
            uint8_t ack:1;                 // is an ACK/NACK expected?
        };
    };

    uint8_t type;             // byte that describes the message to follow
} FRAME_HEADER;

typedef union
{
    int8_t binary_data[MAX_PACKET_SIZE+2];
    struct
    {
        FRAME_HEADER header;
        union
        {
            //char packet_data[MAX_PACKET_SIZE];
            // **** ADD any message types defined above below this line ****
            PACKET_18 data_18;
        };
    };
} FRAME_BUFFER;

extern FRAME_BUFFER rx_buffer[NUM_RX_BUFFER];
extern FRAME_BUFFER tx_buffer[NUM_TX_BUFFER];

void crcInitEngine(void);
unsigned int calculateCRC(int8_t *data_stream, int16_t length);
void crcInitMsg(void);
void crcAddMsg(int8_t *data, int16_t length);
void crcAddByte(int8_t data);
unsigned int crcCalculateMsg(void);

void msgSend(int16_t bnum, int8_t port);
void msgReceive(int16_t bnum, int8_t port);
void msgParse(int16_t bnum, int8_t port);
void msgSendSys(uint8_t msg_count, int8_t port, int8_t type);

// ** END AUTO GENERATED **

#endif	/* MESSAGING_H */

