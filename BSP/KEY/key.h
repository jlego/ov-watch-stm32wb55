#ifndef __KEY_H__
#define __KEY_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"

/* WB55：按键与触摸中断共用 PB2 */
#define KEY1_PORT	GPIOB
#define KEY1_PIN	GPIO_PIN_2
#define KEY1 HAL_GPIO_ReadPin(KEY1_PORT,KEY1_PIN)

void Key_Port_Init(void);
void Key_Interrupt_Callback(void);
uint8_t KeyScan(uint8_t mode);
#ifdef __cplusplus
}
#endif
#endif