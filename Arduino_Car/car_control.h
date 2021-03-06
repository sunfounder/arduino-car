#ifndef __CAR_CONTROL_H__
#define __CAR_CONTROL_H__

#include <Arduino.h>
#include "compass.h"

#define MOTOR_DIRECTIONS (uint8_t[4]){1, 0, 0, 1}
#define MOTOR_PINS       (uint8_t[8]){6, 7, 8, 9, 10, 11, 12, 13}

#define CAR_DEFAULT_POWER 60
#define CAR_ROTATE_POWER 80
#define CAR_CALIBRATION_POWER 80

#define CAR_WIDTH  14.76/2      //定义宽度
#define CAR_LENGTH  11.48/2     //定义长度

// [0]-------[1]
//  |         |
//  |         |
//  |         |
//  |         |
// [3]-------[2]

void carBegin();
// void carSetMotor(uint8_t motor, int8_t power);
void carSetMotors(int8_t power0, int8_t power1, int8_t power2, int8_t power3);
void carForward();
void carBackward();
void carTurnLeft();
void carTurnRight();
void carLeftForword();
void carRightForword();
void carLeftBackward();
void carRightBackward();
void carLeft();
void carRight();
void carStop();
void _carMove(int16_t angle, int8_t power, int8_t rot, bool drift=false);
void carMove(int16_t angle, int8_t power, int8_t rot, bool drift=false);
void carMoveFieldCentric(int16_t angle, int8_t power, int16_t heading, bool drift=false);
void carResetHeading();

#endif // __CAR_CONTROL_H__