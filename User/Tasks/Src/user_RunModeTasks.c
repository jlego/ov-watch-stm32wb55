/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "ui.h"
#include "ui_HomePage.h"
#include "ui_OffTimePage.h"
#include "ui_TimerPage.h"
#include "main.h"
#include "rtc.h"
#include "i2c.h"
#include "spi.h"
#include "stm32wbxx_it.h"
#include "lcd_init.h"
#include "lcd.h"
#include "power.h"
#include <stdio.h>
#include "CST816.h"
#include "MPU6050.h"
#include "key.h"

#include "HWDataAccess.h"

/* SystemClock_Config 定义在 main.c，但未在 main.h 中声明 */
void SystemClock_Config(void);

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint32_t IdleTimerCount = 0;
volatile uint32_t WakeupGraceMs = 0;

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
			/* 宽限期内即使拿到残留的"调暗"消息也不执行，避免刚唤醒就被调暗 */
			if(WakeupGraceMs == 0)
			{
				LCD_Set_Light(5);
				light_off_sent = 1;
			}
		}
		//resume light if light got dark and idle state breaked by key pressing or screen touching
		if(osMessageQueueGet(IdleBreak_MessageQueue,&IdleBreakstr,NULL,1)==osOK)
		{
			IdleTimerCount = 0;
			light_off_sent = 0;
			LCD_Set_Light(ui_LightSliderValue);
		}

		if(WakeupGraceMs)
		{
			/* 唤醒宽限期(5 秒)：把空闲计时一直压在 0，屏幕保持全亮。
			   宽限期结束后再从 0 开始计时，才走正常的"变暗 -> 熄屏"流程。
			   否则用户的变暗时间(ui_LTimeValue)若是 2~3 秒，刚被按键唤醒就立刻变暗，
			   看起来就是"亮一下又黑屏"；熄屏时间(ui_TTimeValue)短的话会直接又睡回去。 */
			IdleTimerCount = 0;
			light_off_sent = 0;
		}
		else
		{
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
	while(1)
	{
		if(osMessageQueueGet(Stop_MessageQueue,&Stopstr,NULL,0)==osOK)
		{

			/*************************** your operations before sleep***************************/
			sleep:
			IdleTimerCount = 0;
			/* 清掉残留的按键唤醒标志，只认进入 STOP 之后产生的按键边沿 */
			Key_ClearWakeUp();

			// RTC WakeUp Timer 关掉：它每 ~1 秒触发 EXTI22，会把 STOP 自动唤醒
			HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
			// 清掉可能残留的 EXTI pending（RTC_WAKEUP=EXTI22，MPU=EXTI15_10）
			__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);

			//sensors

			//lcd
			/* 只让 ST7789 进入睡眠，不做硬件复位：GRAM 内容保留，唤醒后画面能原样恢复 */
			LCD_WR_REG(0x28);
			LCD_ST7789_SleepIn();
			LCD_Close_Light();
			/* touch：不让 CST816 进入深度睡眠(0xE5=0x03 会停止 I2C 响应，唤醒后复位不可靠)。
			   保持其 AutoSleep 低功耗扫描模式即可，该模式功耗仅几 μA 且 I2C 始终响应，
			   唤醒后无需复位，触摸立即可用 */

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
			SystemClock_Config();
			HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq));

			/* 先判断是不是有效唤醒：STOP 期间只要出现过按键边沿就算有效唤醒。
			   无效唤醒(杂散中断)直接回到 STOP，不做后面耗时的外设恢复 */
			if(!(Key_CheckWakeUp() || Key_IsPressed() || ChargeCheck()))
			{
				IdleTimerCount = 0;
				goto sleep;
			}

			/* 下面这些必须在 xTaskResumeAll() 之前做(此时调度器仍挂起，没有任务能插进来)：
			   1) 清空残留消息：STOP 前 IdleEnterTask 可能刚往 Idle_MessageQueue 里放了
			      "调暗亮度"的消息，若被先跑起来的 IdleEnterTask 消费掉，屏幕就会
			      "先亮一下又变黑"；Stop_MessageQueue 里若有残留则会刚唤醒就再睡回去。
			   2) 设置 5 秒唤醒宽限期，期间 IdleEnterTask / MPUCheckTask 都不发 Stop。
			   3) 屏蔽唤醒用的那次按键：按键还按住时不能产生长按事件，否则长按唤醒会被
			      KeyTask 判成 HomePage 长按并立刻熄屏。 */
			{
				uint8_t dummy;
				while(osMessageQueueGet(Idle_MessageQueue, &dummy, NULL, 0) == osOK);
				while(osMessageQueueGet(IdleBreak_MessageQueue, &dummy, NULL, 0) == osOK);
				while(osMessageQueueGet(Stop_MessageQueue, &dummy, NULL, 0) == osOK);
				IdleTimerCount = 0;
				WakeupGraceMs = 5000;
				dummy = 0;
				osMessageQueuePut(IdleBreak_MessageQueue, &dummy, 0, 0);
			}
			Key_AfterWakeup();

			/* 触摸与 I2C 恢复也在调度器挂起期间完成，避免和其它任务的 I2C 访问打架。
			   注意 CST816_RESET() 内部用 HAL_Delay(基于 uwTick)，在调度器挂起时依然
			   有效；而 delay_ms() 走的是 osDelay，调度器挂起时它会立即返回，导致复位
			   脉宽不足、触摸芯片复位不干净，唤醒后就再也读不到触摸数据。 */
			CST816_RESET();
			IIC_Bus_Reset();
			/* SPI1 同理：唤醒后外设可能残留 BSY/OVR，之后所有刷屏传输都会超时返回，
			   画面停在睡眠前那一帧不再更新，看起来就是"有图像但触摸/界面全无反应"。
			   重新初始化一次即可把 SPI 状态机清干净(寄存器在 STOP 期间是保持的) */
			HAL_SPI_Abort(&hspi1);
			MX_SPI1_Init();

			/* 复位过程本身要花上百毫秒，重新计时，让宽限期从"真正恢复运行"开始算 */
			WakeupGraceMs = 5000;
			IdleTimerCount = 0;

			xTaskResumeAll();

			/***********************************************************************************/

			/****************************** your wakeup operations *****************************/

			/* 队列清理、唤醒宽限期、按键屏蔽都已在 xTaskResumeAll() 之前完成 */

			// 先重开 RTC WakeUp Timer（~1 秒周期，保持原硬件配置值）
			HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2000, RTC_WAKEUPCLOCK_RTCCLK_DIV16);

			//MPU Check（只更新 wrist_state，不在唤醒路径里 goto sleep）
			if(HWInterface.IMU.wrist_is_enabled)
			{
				uint8_t hor;
				hor = MPU_isHorizontal();
				if(hor && HWInterface.IMU.wrist_state == WRIST_DOWN)
				{
					HWInterface.IMU.wrist_state = WRIST_UP;
				}
				else if(!hor && HWInterface.IMU.wrist_state == WRIST_UP)
				{
					HWInterface.IMU.wrist_state = WRIST_DOWN;
				}
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
			/* touch：复位后的 CST816 已回到默认配置，先验证 I2C 通路真的通了(读 ChipID)，
			   读不到就再复位一次芯片 + 总线，最多重试 3 次；通了再恢复休眠时间配置 */
			{
				uint8_t touch_retry;
				uint8_t touch_id = 0;
				for(touch_retry = 0; touch_retry < 3; touch_retry++)
				{
					touch_id = CST816_Get_ChipID();
					if(touch_id != 0x00 && touch_id != 0xFF)
					{
						break;
					}
					CST816_RESET();
					IIC_Bus_Reset();
				}
				CST816_Config_AutoSleepTime(5);
			}
			//key
			Key_Port_Init();
			__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
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