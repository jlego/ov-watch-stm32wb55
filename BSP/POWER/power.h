#ifndef _POWER_H_
#define _POWER_H_
#include "stm32wbxx_hal.h"

#define CHARGE_PORT			GPIOA
#define CHARGE_PIN			GPIO_PIN_0

#define BAT_CHECK_PORT	GPIOA
#define BAT_CHECK_PIN		GPIO_PIN_0

void Power_Pins_Init(void);
void Power_Enable(void);
void Power_DisEnable(void);
float BatCheck(void);
float BatCheck_8times(void);
uint8_t ChargeCheck(void);
uint8_t PowerCalculate(void);
void Power_Init(void);
void ADC_Init(void);

extern uint8_t cw2015_ready;

#endif