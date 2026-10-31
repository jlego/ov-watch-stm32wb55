#include "iic_hal.h"
#include "delay.h"

extern I2C_HandleTypeDef hi2c1;
extern void MX_I2C1_Init(void);

/*
*********************************************************************************************************
*	函 数 名: IIC_Bus_Reset
*	功能说明: 完整复位 I2C1 外设并恢复总线
*	形    参: none
*	返 回 值: none
*	备    注: STOP 唤醒后 I2C1 可能残留异常状态：
*	          1) 传输被高优先级任务打断，外设停在半个字节，SCL 被一直拉低；
*	          2) 从机等不到剩余时钟，把 SDA 一直拉低，总线死锁；
*	          3) 以上情况会让 BUSY 标志常置，之后所有 HAL_I2C 调用都直接返回
*	             HAL_BUSY，触摸轮询读到的永远是 0，表现就是“唤醒后触摸失效”。
*	          处理办法：DeInit 释放外设与引脚 -> GPIO 手动打 9 个 SCL 时钟并产生
*	          STOP 条件让从机回到空闲 -> 重新 Init 恢复复用功能与时钟。
*********************************************************************************************************
*/
void IIC_Bus_Reset(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	uint8_t i;

	HAL_I2C_DeInit(&hi2c1);

	/* MspDeInit 会关掉 I2C1 时钟并把引脚交还给 GPIO，这里把 GPIOB 时钟确保打开 */
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* 先释放 SDA，再打 9 个 SCL 时钟，让被卡住的从机把残留的半字节吐完并释放 SDA */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
	for(i = 0; i < 9; i++)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
		delay_us(5);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
		delay_us(5);
	}

	/* 手动产生 STOP 条件：SCL 保持高电平期间 SDA 由低变高 */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
	delay_us(5);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	delay_us(5);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
	delay_us(5);

	/* 重新初始化：此时句柄 State 为 RESET，会重新走 MspInit 恢复时钟与引脚复用 */
	MX_I2C1_Init();
}

void IICInit(iic_bus_t *bus)
{
	bus->Instance = &hi2c1;
}

void IICStart(iic_bus_t *bus)
{
}

void IICStop(iic_bus_t *bus)
{
}

unsigned char IICWaitAck(iic_bus_t *bus)
{
	return SUCCESS;
}

void IICSendAck(iic_bus_t *bus)
{
}

void IICSendNotAck(iic_bus_t *bus)
{
}

void IICSendByte(iic_bus_t *bus,unsigned char cSendByte)
{
}

unsigned char IICReceiveByte(iic_bus_t *bus)
{
	return 0;
}

uint8_t IIC_Write_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t data)
{
	if(HAL_I2C_Mem_Write(bus->Instance, daddr<<1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, IIC_TIMEOUT) != HAL_OK)
	{
		return 1;
	}
	return 0;
}

uint8_t IIC_Write_Multi_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t length,uint8_t buff[])
{
	if(HAL_I2C_Mem_Write(bus->Instance, daddr<<1, reg, I2C_MEMADD_SIZE_8BIT, buff, length, IIC_TIMEOUT) != HAL_OK)
	{
		return 1;
	}
	return 0;
}

unsigned char IIC_Read_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg)
{
	unsigned char dat = 0;
	HAL_I2C_Mem_Read(bus->Instance, daddr<<1, reg, I2C_MEMADD_SIZE_8BIT, &dat, 1, IIC_TIMEOUT);
	return dat;
}

uint8_t IIC_Read_Multi_Byte(iic_bus_t *bus, uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[])
{
	if(HAL_I2C_Mem_Read(bus->Instance, daddr<<1, reg, I2C_MEMADD_SIZE_8BIT, buff, length, IIC_TIMEOUT) != HAL_OK)
	{
		return 1;
	}
	return 0;
}