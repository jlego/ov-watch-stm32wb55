#include "power.h"
#include "delay.h"

#define INTERNAL_RES 0.128
#define CHARGING_CUR 1

#define ADC_VREF 3.3f
#define ADC_FULLSCALE 4095.0f

extern ADC_HandleTypeDef hadc1;

static uint8_t adc_ready = 0;

void Power_Pins_Init()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PA0 充电检测输入 */
  GPIO_InitStruct.Pin = CHARGE_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(CHARGE_PORT, &GPIO_InitStruct);
}

void Power_Enable()
{
}

void Power_DisEnable()
{
}

uint8_t ChargeCheck()//1:charging
{
	return HAL_GPIO_ReadPin(CHARGE_PORT,CHARGE_PIN);
}

void ADC_Init()
{
	ADC_ChannelConfTypeDef sConfig = {0};

	__HAL_RCC_ADC_CLK_ENABLE();

	/* 64MHz 时钟下 ADC 最大 16MHz，使用 PLLSAI1R 作为异步时钟源 */
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
	PeriphClkInit.PLLSAI1.PLLN = 16;
	PeriphClkInit.PLLSAI1.PLLP = RCC_PLLP_DIV2;
	PeriphClkInit.PLLSAI1.PLLQ = RCC_PLLQ_DIV4;
	PeriphClkInit.PLLSAI1.PLLR = RCC_PLLR_DIV4;
	PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_ADCCLK;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}

	hadc1.Instance = ADC1;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
	hadc1.Init.OversamplingMode = DISABLE;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	/* PA0 -> ADC1_IN5，电池分压输入 */
	sConfig.Channel = ADC_CHANNEL_5;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}

	/* 校准需在 ADC 使能前完成 */
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	adc_ready = 1;
}

static void ADC_Pin_To_Analog(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = BAT_CHECK_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BAT_CHECK_PORT, &GPIO_InitStruct);
}

static void ADC_Pin_To_PWM(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = BAT_CHECK_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
	HAL_GPIO_Init(BAT_CHECK_PORT, &GPIO_InitStruct);
}

static uint16_t ADC_ReadRaw(void)
{
	uint16_t dat = 0;
	if(!adc_ready)
	{
		return 0;
	}
	/* PA0 与背光 PWM 复用，采样前切换为模拟输入 */
	ADC_Pin_To_Analog();
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,5);
	dat = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	ADC_Pin_To_PWM();
	return dat;
}

float BatCheck()
{
	uint16_t dat;
	float BatVoltage;
	dat = ADC_ReadRaw();
	BatVoltage = dat *2 *ADC_VREF /ADC_FULLSCALE;
	return BatVoltage;
}

float BatCheck_8times()
{
	uint32_t dat=0;
	uint8_t i;
	float BatVoltage;
	for(i=0;i<8;i++)
	{
		dat += ADC_ReadRaw();
		delay_ms(1);
	}
	dat = dat>>3;
	BatVoltage = dat *2 *ADC_VREF /ADC_FULLSCALE;
	return BatVoltage;
}

uint8_t PowerCalculate()
{
	uint8_t power = 0;
	float voltage;
	voltage = BatCheck_8times();

	if(ChargeCheck())
	{voltage -= INTERNAL_RES * CHARGING_CUR;}

	if((voltage >= 4.2))
	{power = 100;}
	else if(voltage >= 4.06 && voltage <4.2)
	{power = 90;}
	else if(voltage >= 3.98 && voltage <4.06)
	{power = 80;}
	else if(voltage >= 3.92 && voltage <3.98)
	{power = 70;}
	else if(voltage >= 3.87 && voltage <3.92)
	{power = 60;}
	else if(voltage >= 3.82 && voltage <3.87)
	{power = 50;}
	else if(voltage >= 3.79 && voltage <3.82)
	{power = 40;}
	else if(voltage >= 3.77 && voltage <3.79)
	{power = 30;}
	else if(voltage >= 3.74 && voltage <3.77)
	{power = 20;}
	else if(voltage >= 3.68 && voltage <3.74)
	{power = 10;}
	else if(voltage >= 3.45 && voltage <3.68)
	{power = 5;}
	return power;
}

void Power_Init(void)
{
	Power_Pins_Init();
	Power_Enable();
}