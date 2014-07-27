/* mmri.h
 * Peter Klein
 * Created on November 3, 2013, 6:50 PM
 * Description:
 *
 */

#ifndef MMRI_H
#define	MMRI_H

// Number of variables in memory map !! NOTE: MAX OF 256 (8-bit value) !!
#define MMRI_NUM    256

#if MMRI_NUM > 256
#error "Number of memory map variables too big (256 max)"
#endif

// Allowable variable types for a register
#define UNDEF       0
#define UINT8       1
#define INT8        2
#define UINT16      3
#define INT16       4
#define UINT32      5
#define INT32       6
#define UINT64      7
#define INT64       8
#define FLOAT       9
#define STRING      10       // strings are 20 characters long!

// Defines for variable init
#define RO          0
#define RW          1
#define VOL         0
#define NVM         1
#define NOPW        0
#define PWWR        1
#define PWRD        2

// Defines for error codes
#define NOERROR     0       // All Good!
#define UNKNOWN     1       // Unknown error has occured
#define BADCS       2       // Bad checksum
#define BADADDR     3       // Address not used, recognized, read-only, or out of range
#define BADVAL      4       // Value too big, small, long, negative, or NAN
#define BADPASS     5       // Bad password entered, or not privileged to access

// Structure of a single register
typedef struct
{
    union
    {
        uint8_t config;
        struct
        {
            uint8_t used:1;     // Set to 1 if used. If not used, reading will error
            uint8_t rw:1;       // 0 = read-only, 1 = read/write
            uint8_t nvm:1;      // 1 = variable can/should be saved in NVM
            uint8_t pwp:2;      // 1 = can only be written with password unlock
                                // 2 = can only be read/written with password unlock
        };
    };
    uint8_t type;       // what type of variable is this. See defines above
    void *ptr;          // pointer to the variable
} MMRIreg;

typedef union
{
  uint64_t num;
  uint8_t bytes[8];
} int64Bytes;

void mmriInit(void);
void mmriSaveNVM();
void mmriReadNVM();
void mmriInitVar(uint8_t addr, uint8_t type, uint8_t rw, uint8_t nvm, uint8_t pwp, void *ptr);
void *mmriGetRegPtr(uint8_t addr);
void mmriPrintReg(uint8_t addr);
uint8_t mmriWriteRegAscii(uint8_t addr, char *value);
void mmriPrintAllReg(void);

#endif	/* MMRI_H */

