/* system.h
 * Peter Klein
 * Created on March 3, 2013, 8:06 PM
 * Description:
 *
 * This file contains #defines for use throughout the code. Oscillator frequency,
 * special I/O pins, etc. should be defined here. Also contains function prototypes
 * for system.c, which relate to system wide functions.
 *
 * Naming conventions:
 * functionName(), awesome_variable, p_ointer_variable, STATIC_DEFINE
 */

#ifndef SYSTEM_H
#define	SYSTEM_H

// Include the microcontroller
#include "p33EP512MU810.h"
#include "stdint.h"

/** Oscillator related *********************************************/


/** Special I/O pins ***********************************************/
#define CS_SPI0                 PORTAbits.RA0

/** LEDs ***********************************************************/
#define LED1                    LATDbits.LATD0
#define LED2                    LATDbits.LATD1
#define LED3                    LATDbits.LATD2

#define led1On()                LED1 = 1      // assumes LEDs are wired
#define led2On()                LED2 = 1      // to sink current through
#define led3On()                LED3 = 1      // the microcontroller

#define led1Off()               LED1 = 0
#define led2Off()               LED2 = 0
#define led3Off()               LED3 = 0

#define led1Toggle()            LED1 = !LED1
#define led2Toggle()            LED2 = !LED2
#define led3Toggle()            LED3 = !LED3

/** SWITCHES *******************************************************/
#define SWITCH1                 PORTEbits.RE0
#define SWITCH2                 PORTEbits.RE1
#define SWITCH3                 PORTEbits.RE2

/** Basic definitions **********************************************/
#define ON                      1
#define OFF                     0
#define HIGH                    1
#define LOW                     0
#define TRUE                    1
#define FALSE                   0
#define YES                     1
#define NO                      0
#define NULL                    0

#define reset()                  __asm__ volatile("reset")
#define disiOn()                __asm__ volatile("disi #0x3FFF")
#define disiOff()               __asm__ volatile("disi #0x0000")
#define disableInterrupts()     INTCON2bits.GIE = 0
#define enableInterrupts()      INTCON2bits.GIE = 1

// Function declarations
void changeClockFreq(unsigned char mhz);
unsigned long getClockFreq(void);

#endif	/* SYSTEM_H */

