/* Private includes -----------------------------------------------------------*/
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "main.h"
#include "lvgl.h"
#include "ui_HomePage.h"
#include "ui_MenuPage.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern osMessageQueueId_t Key_MessageQueue;

volatile uint8_t Key_RequestPageBack = 0;

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Screen renew task - 只收按键消息，设标志位，不调 LVGL API
  * @param  argument: Not used
  * @retval None
  */
void ScrRenewTask(void *argument)
{
	uint8_t keystr = 0;
	while(1)
	{
		if(osMessageQueueGet(Key_MessageQueue, &keystr, NULL, 0) == osOK)
		{
			if(keystr == 1 || keystr == 2)
			{
				Key_RequestPageBack = 1;
			}
		}
		osDelay(1);
	}
}