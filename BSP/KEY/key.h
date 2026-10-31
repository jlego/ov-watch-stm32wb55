#ifndef __KEY_H__
#define __KEY_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"

/* WB55：按键接在 PH3 */
#define KEY1_PORT	GPIOH
#define KEY1_PIN	GPIO_PIN_3
#define KEY1 HAL_GPIO_ReadPin(KEY1_PORT,KEY1_PIN)

uint8_t Key_IsPressed(void);
uint8_t Key_CheckWakeUp(void);
void Key_ClearWakeUp(void);

void Key_Port_Init(void);
void Key_Interrupt_Callback(void);
uint8_t KeyScan(uint8_t mode);
#ifdef __cplusplus
}
#endif
#endif