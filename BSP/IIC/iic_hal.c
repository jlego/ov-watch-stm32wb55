#include "iic_hal.h"

extern I2C_HandleTypeDef hi2c1;

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