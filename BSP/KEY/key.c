#include "key.h" 
#include "user_TasksInit.h"

/* 按键按下时的电平：GPIO_PIN_RESET=按键接地，GPIO_PIN_SET=按键接VCC */
static uint8_t Key_ActiveLevel = GPIO_PIN_RESET;

/* 按键唤醒标志：只要在 STOP 期间出现过按键边沿就置位，避免“按一下就松手”被判为无效唤醒 */
static volatile uint8_t Key_WakeUpFlag = 0;

void Key_Port_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOH_CLK_ENABLE();

  /* PH3 按键接地：上拉输入，下降沿 EXTI 唤醒 */
  Key_ActiveLevel = GPIO_PIN_RESET;
  GPIO_InitStruct.Pin = KEY1_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(KEY1_PORT, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

uint8_t Key_IsPressed(void)
{
	return (KEY1 == Key_ActiveLevel) ? 1 : 0;
}

/* EXTI3 中断回调(由 EXTI3_IRQHandler -> HAL_GPIO_EXTI_IRQHandler 调用) */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == KEY1_PIN)
	{
		Key_WakeUpFlag = 1;
	}
}

uint8_t Key_CheckWakeUp(void)
{
	if(Key_WakeUpFlag)
	{
		Key_WakeUpFlag = 0;
		return 1;
	}
	return 0;
}

void Key_ClearWakeUp(void)
{
	Key_WakeUpFlag = 0;
}

#define KEY_LONG_PRESS_MS   1500

uint8_t KeyScan(uint8_t mode)
{
	static uint8_t key_up = 1;
	static uint8_t key_down = 0;
	static uint8_t long_sent = 0;
	static uint32_t press_tick = 0;
	uint8_t keyvalue = 0;

	if(mode)
	{
		key_up = 1;
		key_down = 0;
		long_sent = 0;
	}

	if( key_up && Key_IsPressed() )
	{
		osDelay(3);
		if(Key_IsPressed())
		{
			key_down = 1;
			key_up = 0;
			press_tick = HAL_GetTick();
			long_sent = 0;
		}
	}

	if( key_down && !long_sent && (HAL_GetTick() - press_tick >= KEY_LONG_PRESS_MS) )
	{
		long_sent = 1;
		keyvalue = 2;
	}

	if ( key_down && !Key_IsPressed() )
	{
		osDelay(3);
		if(!Key_IsPressed())
		{
			key_up = 1;
			if(!long_sent)
				keyvalue = 1;
			key_down = 0;
			long_sent = 0;
		}
	}

	return keyvalue;
}