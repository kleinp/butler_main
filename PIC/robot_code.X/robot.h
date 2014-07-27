/* robot.h
 * Peter Klein
 * Created on August 18, 2013, 1:44 PM
 * Description:
 *
 */

#ifndef ROBOT_H
#define	ROBOT_H

#define LEFT                1
#define RIGHT               2

typedef union
{
    int32_t val;
    int16_t val16[2];
} encoder32;

void robotInit(void);
void tasks100Hz(void);
void tasks50Hz(void);
void tasks10Hz(void);
void tasks5Hz(void);
void tasks1Hz(void);
void initMotorPWM(void);
void setMotorOutputs(int8_t left, int8_t right);

#endif	/* ROBOT_H */

