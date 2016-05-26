#ifndef _LSM330_
#define _LSM330_

/* 3-axis Accelerometer/Gyroscope LSM330 by ST electronics

 Register map according to datasheet:     */

#define CTRL1_REG1_A   0x20
#define STATUS_REG_A   0x27
#define OUT_X_L_A      0x28
#define OUT_X_H_A	   0x29
#define OUT_Y_L_A      0x2A
#define OUT_Y_H_A	   0x2B
#define OUT_Z_L_A      0x2C
#define OUT_Z_H_A	   0x2D

#define WHO_AM_I_G	   0x0F

#endif