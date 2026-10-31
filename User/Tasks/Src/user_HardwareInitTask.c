/* Private includes -----------------------------------------------------------*/

// includes
// sys
#include "tim.h"
#include "rtc.h"
#include "stm32wbxx_it.h"
#include "delay.h"

// user
#include "user_TasksInit.h"
#include "HWDataAccess.h"
#include "version.h"

// bsp
#include "key.h"
#include "lcd.h"
#include "lcd_init.h"
#include "CST816.h"
#include "DataSave.h"
#include "power.h"
#include "cw2015.h"

// ui
//gui
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui.h"

// APP SYS setting
#include "ui_DateTimeSetPage.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  hardwares init task
  * @param  argument: Not used
  * @retval None
  */
void HardwareInitTask(void *argument)
{
	while(1)
	{
    vTaskSuspendAll();

    if(HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2000, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
    {
      Error_Handler();
    }

    HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
    delay_init();

    LCD_Init();
    LCD_Fill(0,0, LCD_W, LCD_H, BLACK);
    delay_ms(10);
    LCD_Set_Light(50);
    LCD_ShowString(72,LCD_H/2,(uint8_t*)"Welcome!", WHITE, BLACK, 24, 0);

    CST816_GPIO_Init();
    CST816_RESET();

    Key_Port_Init();

    uint8_t lcd_buf_str[17];
    sprintf(lcd_buf_str, "OV-Watch V%d.%d.%d", watch_version_major(), watch_version_minor(), watch_version_patch());
    LCD_ShowString(34, LCD_H/2+48, (uint8_t*)lcd_buf_str, WHITE, BLACK, 24, 0);

    {
      uint8_t dbg_str[24];
      sprintf(dbg_str, "CW:%c v=%02X m=%02X S=%u",
        cw2015_dbg_init_ok ? 'Y' : 'N',
        cw2015_dbg_version,
        cw2015_dbg_mode,
        cw2015_dbg_soc_raw / 256);
      LCD_ShowString(10, LCD_H-30, dbg_str, WHITE, BLACK, 16, 0);
    }

    delay_ms(1000);
    LCD_Fill(0, LCD_H/2-24, LCD_W, LCD_H/2+49, BLACK);

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    ui_init();

    xTaskResumeAll();
		vTaskDelete(NULL);
		osDelay(500);
	}
}