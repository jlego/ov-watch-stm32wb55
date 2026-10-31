#include "cw2015.h"
#include "delay.h"

#define CW2015_I2C_TIMEOUT 100

static I2C_HandleTypeDef *cw2015_i2c = NULL;

static uint8_t cw2015_profile[20] = {
	0x15, 0x03, 0x14, 0x08, 0x6A, 0x3D, 0x3B, 0x35,
	0x32, 0x2F, 0x2B, 0x27, 0x22, 0x1D, 0x18, 0x10,
	0x07, 0x02, 0x00, 0x00
};

static uint8_t cw2015_read_reg(uint8_t reg)
{
	uint8_t dat = 0xFF;
	if(cw2015_i2c == NULL) return 0xFF;
	HAL_I2C_Mem_Read(cw2015_i2c, CW2015_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, &dat, 1, CW2015_I2C_TIMEOUT);
	return dat;
}

static uint8_t cw2015_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
	if(cw2015_i2c == NULL) return 1;
	if(HAL_I2C_Mem_Read(cw2015_i2c, CW2015_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, CW2015_I2C_TIMEOUT) != HAL_OK)
	{
		return 1;
	}
	return 0;
}

static uint8_t cw2015_write_reg(uint8_t reg, uint8_t data)
{
	if(cw2015_i2c == NULL) return 1;
	if(HAL_I2C_Mem_Write(cw2015_i2c, CW2015_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, CW2015_I2C_TIMEOUT) != HAL_OK)
	{
		return 1;
	}
	return 0;
}

static uint8_t cw2015_write_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
	if(cw2015_i2c == NULL) return 1;
	if(HAL_I2C_Mem_Write(cw2015_i2c, CW2015_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, CW2015_I2C_TIMEOUT) != HAL_OK)
	{
		return 1;
	}
	return 0;
}

static uint8_t cw2015_load_profile(void)
{
	uint8_t i;
	uint8_t checksum = 0;

	for(i = 0; i < 18; i++)
	{
		checksum += cw2015_profile[i];
	}
	cw2015_profile[18] = checksum;

	if(cw2015_write_regs(CW2015_REG_CONFIG, (uint8_t *)cw2015_profile, 20) != 0)
	{
		return 1;
	}

	cw2015_write_reg(CW2015_REG_CMD, CW2015_CMD_RESTART);
	delay_ms(100);

	return 0;
}

uint8_t CW2015_Init(I2C_HandleTypeDef *hi2c)
{
	cw2015_i2c = hi2c;

	uint8_t version = cw2015_read_reg(CW2015_REG_VERSION);
	if(version == 0xFF || version == 0x00)
	{
		return 1;
	}

	CW2015_Wakeup();
	delay_ms(10);

	cw2015_load_profile();
	delay_ms(500);

	return 0;
}

float CW2015_GetVoltage(void)
{
	uint8_t buf[2];
	if(cw2015_read_regs(CW2015_REG_VCELL, buf, 2) != 0)
	{
		return 0.0f;
	}
	uint16_t vcell_raw = ((uint16_t)buf[0] << 8) | buf[1];
	return (vcell_raw * 3.984375f) / 1000.0f;
}

float CW2015_GetSOC(void)
{
	uint8_t buf[2];
	if(cw2015_read_regs(CW2015_REG_SOC, buf, 2) != 0)
	{
		return 0.0f;
	}
	uint16_t soc_raw = ((uint16_t)buf[0] << 8) | buf[1];
	return soc_raw / 256.0f;
}

uint8_t CW2015_GetSOCInt(void)
{
	float soc = CW2015_GetSOC();
	if(soc > 100.0f) soc = 100.0f;
	if(soc < 0.0f) soc = 0.0f;
	return (uint8_t)(soc + 0.5f);
}

uint8_t CW2015_Sleep(void)
{
	return cw2015_write_reg(CW2015_REG_MODE, CW2015_MODE_SLEEP);
}

uint8_t CW2015_Wakeup(void)
{
	return cw2015_write_reg(CW2015_REG_MODE, CW2015_MODE_NORMAL);
}

uint8_t CW2015_QuickStart(void)
{
	cw2015_write_reg(CW2015_REG_MODE, CW2015_MODE_QUICKSTART);
	delay_ms(500);
	cw2015_write_reg(CW2015_REG_MODE, CW2015_MODE_NORMAL);
	return 0;
}