#ifndef CW2015_H
#define CW2015_H

#include "stm32wbxx_hal.h"

#define CW2015_ADDR 0x62

#define CW2015_REG_VERSION 0x00
#define CW2015_REG_VCELL   0x02
#define CW2015_REG_SOC     0x04
#define CW2015_REG_MODE    0x06
#define CW2015_REG_CONFIG  0x08
#define CW2015_REG_CMD     0xFE

#define CW2015_MODE_SLEEP      0x00
#define CW2015_MODE_NORMAL     0x01
#define CW2015_MODE_QUICKSTART 0x03

#define CW2015_CMD_RESTART 0xA5

uint8_t CW2015_Init(I2C_HandleTypeDef *hi2c);
float CW2015_GetVoltage(void);
float CW2015_GetSOC(void);
uint8_t CW2015_GetSOCInt(void);
uint8_t CW2015_Sleep(void);
uint8_t CW2015_Wakeup(void);
uint8_t CW2015_QuickStart(void);

#endif