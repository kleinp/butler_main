/* 
 * File:   nvm.h
 * Author: Peter
 *
 * Created on January 1, 2014, 5:12 PM
 */

#ifndef NVM_H
#define	NVM_H

void nvmInit(void);
void nvmWriteByte(uint8_t addr, uint8_t data);
uint8_t nvmReadByte(uint8_t addr);
void nvmWriteArray(uint8_t addr, uint16_t length, uint8_t *data);
void nvmReadArray(uint8_t addr, uint16_t length, uint8_t *data);

#define nvmSTART        0
#define nvmWRITE        1
#define nvmWAIT         2
#define nvmSTOP         3

#endif	/* NVM_H */

