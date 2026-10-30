#include "key.h"
#include "delay.h"
#include "user_TasksInit.h"

void Key_Port_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PB2 (与触摸中断共用) */
  GPIO_InitStruct.Pin = KEY1_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY1_PORT, &GPIO_InitStruct);
}

uint8_t KeyScan(uint8_t mode)
{
	static uint8_t key_up = 1;
	static uint8_t key_down = 0;
	uint8_t keyvalue = 0;

	if(mode)
	{
		key_up = 1;
		key_down = 0;
	}

	if( key_up && (!KEY1) )
	{
		osDelay(3);//ensure the key
		if(!KEY1)
			key_down = 1;
		if(key_down)
			key_up = 0;
	}

	if ( key_down && KEY1 )
	{
		osDelay(3);//ensure the key
		if(KEY1)
		{
			key_up = 1;
			keyvalue = key_down;
			key_down = 0;
		}
	}

	return keyvalue;
}