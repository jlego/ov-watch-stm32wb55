/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "ui_HomePage.h"
#include "main.h"
#include "key.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Key press check task
  * @param  argument: Not used
  * @retval None
  */
void KeyTask(void *argument)
{
	uint8_t keystr=0;
	uint8_t Stopstr=0;
	uint8_t IdleBreakstr=0;
	uint8_t last_pressed = 0;
	while(1)
	{
		uint8_t now_pressed = Key_IsPressed();

		if(now_pressed && !last_pressed)
		{
			osDelay(3);
			if(Key_IsPressed())
			{
				if(Page_Get_NowPage()->page_obj != &ui_HomePage)
				{
					keystr = 1;
					osMessageQueuePut(Key_MessageQueue, &keystr, 0, 0);
					osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 0);
				}
			}
		}
		last_pressed = now_pressed;

		switch(KeyScan(0))
		{
			case 1:
				osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 0);
				break;

			case 2:
				if(Page_Get_NowPage()->page_obj == &ui_HomePage)
				{
					osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 1);
				}
				else
				{
					osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 0);
				}
				break;
		}
		osDelay(1);
	}
}