/* hardware.h
 * Peter Klein
 * Created on August 18, 2013, 1:54 PM
 * Description:
 *
 */

#ifndef HARDWARE_H
#define	HARDWARE_H

#include <stdint.h>

// Maximum value for output compare. Clock freq (132,660,000/20,000) ~ 3316.5
// which I'm rounding to 3350. DIV value is MAX_VAL divided by 50
// which is the number of speeds that are allowed.

#define MAX_OC_VAL          3350
#define MAX_OC_VAL_DIV      67

#define LEFT                1
#define RIGHT               2

typedef union
{
    int64_t position;
    int16_t reg_vals[4];
} ENC64;


void initEncoder(void);
int64_t getEncPos(char side);

#endif	/* HARDWARE_H */

