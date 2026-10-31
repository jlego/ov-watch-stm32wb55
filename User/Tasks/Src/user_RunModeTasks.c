/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"

#include "ui.h"
#include "ui_HomePage.h"
#include "ui_OffTimePage.h"
#include "ui_TimerPage.h"

#include "main.h"
#include "stm32wbxx_it.h"
#include "lcd_init.h"
#include "power.h"
#include "CST816.h"
#include "MPU6050.h"
#include "key.h"

#include "HWDataAccess.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint32_t IdleTimerCount = 0;

/* Private function prototypes -----------------------------------------------*/

/* Tasks ---------------------------------------------------------------------*/

/**
	* @brief  Enter Idle state
	* @param  argument: Not used
	* @retval None
	*/
void IdleEnterTask(void *argument)
{
	uint8_t Idlestr=0;
	uint8_t IdleBreakstr=0;
	uint8_t light_off_sent = 0;
	while(1)
	{
		//light get dark
		if(osMessageQueueGet(Idle_MessageQueue,&Idlestr,NULL,1)==osOK)
		{
			LCD_Set_Light(5);
			light_off_sent = 1;
		}
		//resume light if light got dark and idle state breaked by key pressing or screen touching
		if(osMessageQueueGet(IdleBreak_MessageQueue,&IdleBreakstr,NULL,1)==osOK)
		{
			IdleTimerCount = 0;
			light_off_sent = 0;
			LCD_Set_Light(ui_LightSliderValue);
		}

		if((!light_off_sent) && (IdleTimerCount >= ((uint32_t)ui_LTimeValue * 1000U)))
		{
			uint8_t IdleMsg = 0;
			osMessageQueuePut(Idle_MessageQueue, &IdleMsg, 0, 0);
		}

		if(IdleTimerCount >= ((uint32_t)ui_TTimeValue * 1000U))
		{
			uint8_t Stopstr = 1;
			IdleTimerCount = 0;
			light_off_sent = 0;
			osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 0);
		}

		osDelay(10);
	}
}

/**
  * @brief  enter the stop mode and resume
  * @param  argument: Not used
  * @retval None
  */
void StopEnterTask(void *argument)
{
	uint8_t Stopstr;
	uint8_t HomeUpdataStr;
	uint8_t Wrist_Flag=0;
	while(1)
	{
		if(osMessageQueueGet(Stop_MessageQueue,&Stopstr,NULL,0)==osOK)
		{

			/*************************** your operations before sleep***************************/
			sleep:
			IdleTimerCount = 0;
			/* 清掉残留的按键唤醒标志，只认进入 STOP 之后产生的按键边沿 */
			Key_ClearWakeUp();

			//sensors

			//lcd
			/* 只让 ST7789 进入睡眠，不做硬件复位：GRAM 内容保留，唤醒后画面能原样恢复 */
			LCD_WR_REG(0x28);
			LCD_ST7789_SleepIn();
			LCD_Close_Light();
			//touch
			CST816_Sleep();

			/***********************************************************************************/

			/****************************** enter wakeup operations *****************************/

			vTaskSuspendAll();
			//systick int
			CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
			//enter stop mode
			HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON,PWR_STOPENTRY_WFI);

			//here is the sleep period

			/***********************************************************************************/

			/****************************** quit wakeup operations *****************************/

			//resume run mode and reset the sysclk
			SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
			HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq));
			SystemClock_Config();
			xTaskResumeAll();

			/***********************************************************************************/

			/****************************** your wakeup operations *****************************/

			//MPU Check
			if(HWInterface.IMU.wrist_is_enabled)
			{
				uint8_t hor;
				hor = MPU_isHorizontal();
				if(hor && HWInterface.IMU.wrist_state == WRIST_DOWN)
				{
					HWInterface.IMU.wrist_state = WRIST_UP;
					Wrist_Flag = 1;
					//resume, go on
				}
				else if(!hor && HWInterface.IMU.wrist_state == WRIST_UP)
				{
					HWInterface.IMU.wrist_state = WRIST_DOWN;
					IdleTimerCount  = 0;
					goto sleep;
				}
			}

			//
			/* Key_CheckWakeUp()：STOP 期间出现过按键边沿即认为有效唤醒(按一下松手也不会被漏掉) */
			if(Key_CheckWakeUp() || Key_IsPressed() || ChargeCheck() || Wrist_Flag)
			{
				Wrist_Flag = 0;
				//resume, go on
			}
			else
			{
				IdleTimerCount  = 0;
				goto sleep;
			}

			//usart
			//lcd
			/* 退出 ST7789 睡眠并重新开显示，GRAM 未丢，不需要重跑 LCD_Init() */
			LCD_ST7789_SleepOut();
			LCD_WR_REG(0x13);
			LCD_WR_REG(0x29);
			delay_ms(50);
			LCD_Open_Light();
			LCD_Set_Light(ui_LightSliderValue);
			//touch
			CST816_Wakeup();
			/* STOP 唤醒后重配 PH3 GPIO/EXTI，确保按键电平、Key_ActiveLevel、
			   EXTI 配置全部干净（此时距唤醒已 300+ms，用户早松手了，采样不会错） */
			Key_Port_Init();
			//check if is Charging
			if(ChargeCheck())
			{osEventFlagsSet(HardIntEventHandle, HARDINT_EVENT_CHARG);}
			//send the Home Updata message
			osMessageQueuePut(HomeUpdata_MessageQueue, &HomeUpdataStr, 0, 1);

			/**************************************************************************************/

		}
		osDelay(100);
	}
}