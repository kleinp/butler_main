/* lsm330dlc.h
 * Peter Klein
 * Created on April 28, 2013, 7:42 PM
 * Description:
 *
 */

#ifndef LSM330DLC_H
#define	LSM330DLC_H

#include <stdint.h>

#define ACCEL           0
#define GYRO            1

#define CS_A            LATDbits.LATD2
#define CS_G            LATDbits.LATD1

#define lsmCsaLow()     CS_A = 0
#define lsmCsaHigh()    CS_A = 1

#define lsmCsgLow()     CS_G = 0
#define lsmCsgHigh()    CS_G = 1

#define SPIGCON1bits    SPI1CON1bits
#define SPIGCON2bits    SPI1CON2bits
#define SPIGSTATbits    SPI1STATbits
#define SPIGBUF         SPI1BUF

#define SPIACON1bits    SPI3CON1bits
#define SPIACON2bits    SPI3CON2bits
#define SPIASTATbits    SPI3STATbits
#define SPIABUF         SPI3BUF

#define A_SCALE_FACTOR  16384.0
#define G_SCALE_FACTOR  131.072

// Register addresses of the LSM330DLC
#define WHO_AM_I_G      0x0F
#define CTRL_REG1_G     0x20
#define CTRL_REG2_G     0x21
#define CTRL_REG3_G     0x22
#define CTRL_REG4_G     0x23
#define CTRL_REG5_G     0x24
#define REFERENCE_G     0x25
#define OUT_TEMP_G      0x26
#define STATUS_REG_G    0x27
#define OUT_X_L_G       0x28
#define OUT_X_H_G       0x29
#define OUT_Y_L_G       0x2A
#define OUT_Y_H_G       0x2B
#define OUT_Z_L_G       0x2C
#define OUT_Z_H_G       0x2D

#define CTRL_REG1_A     0x20
#define CTRL_REG2_A     0x21
#define CTRL_REG3_A     0x22
#define CTRL_REG4_A     0x23
#define CTRL_REG5_A     0x24
#define CTRL_REG6_A     0x25
#define REFERENCE_A     0x26
#define STATUS_REG_A    0x27
#define OUT_X_L_A       0x28
#define OUT_X_H_A       0x29
#define OUT_Y_L_A       0x2A
#define OUT_Y_H_A       0x2B
#define OUT_Z_L_A       0x2C
#define OUT_Z_H_A       0x2D

typedef union
{
    int8_t binary_data[8];
    struct
    {
        uint8_t status_reg_a;
        uint8_t out_x_l_a;
        uint8_t out_x_h_a;
        uint8_t out_y_l_a;
        uint8_t out_y_h_a;
        uint8_t out_z_l_a;
        uint8_t out_z_h_a;
        uint8_t dummy;          // for alignment
    };
} LSM_A_RAW;

typedef union
{
    int8_t binary_data[8];
    struct
    {
        uint8_t status_reg_g;
        uint8_t out_x_l_g;
        uint8_t out_x_h_g;
        uint8_t out_y_l_g;
        uint8_t out_y_h_g;
        uint8_t out_z_l_g;
        uint8_t out_z_h_g;
        uint8_t dummy;          // for alignment
    };
} LSM_G_RAW;

typedef struct
{
    uint8_t counter;        // just to keep track of samples
    uint8_t status;         // status from the LSM
    int16_t ax_cnts;        // raw counts, signed
    int16_t ay_cnts;
    int16_t az_cnts;
    int16_t gx_cnts;
    int16_t gy_cnts;
    int16_t gz_cnts;
    
    int16_t ax;             // Acceleration
    int16_t ay;             // 100ths of m/s^2
    int16_t az;
    int16_t gx;             // Angular rates
    int16_t gy;             // 10ths of deg/s
    int16_t gz;

} LSM_DATA;


void lsmInit(void);
void lsmReadMotionData(void);

#endif	/* LSM330DLC_H */
