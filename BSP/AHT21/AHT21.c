#include "AHT21.h"

#define AHT_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE()
iic_bus_t AHT_bus =
{
	.Instance = &hi2c1
};

uint8_t AHT_Read_Status(void)
{
	uint8_t Byte_first = 0;
	HAL_I2C_Master_Receive(AHT_bus.Instance, 0x71, &Byte_first, 1, 100);
	return Byte_first;
}

uint8_t AHT_Read_Cal_Enable(void)  //check cal enable bit 
{
	uint8_t val = 0;//ret = 0,
 
  val = AHT_Read_Status();
  if((val & 0x68)==0x08)  //check NOR mode 
		return 1;
  else  
		return 0;
}

void AHT_Reset(void)//AHT21 send 0xBA reset call
{
	uint8_t cmd = 0xBA;
	HAL_I2C_Master_Transmit(AHT_bus.Instance, 0x70, &cmd, 1, 100);
}

uint8_t AHT_Init(void)
{
	AHT_CLK_ENABLE;
	IICInit(&AHT_bus);
	
	delay_ms(40);
	
	if(AHT_Read_Status()&&0x08!=0x08)
	{
		uint8_t cmd[3] = {0xBE, 0x08, 0x00};
		HAL_I2C_Master_Transmit(AHT_bus.Instance, 0x70, cmd, 3, 100);
		delay_ms(10);
	}
	//AHT_Reset();
	
	return 0;
}	
 
uint8_t AHT_Read(float *humi, float *temp)
{
	uint8_t cnt=5;
	uint8_t  Byte_1th=0;
	uint8_t  Byte_2th=0;
	uint8_t  Byte_3th=0;
	uint8_t  Byte_4th=0;
	uint8_t  Byte_5th=0;
	uint8_t  Byte_6th=0;
	uint32_t RetuData = 0;
	
	uint8_t cmd[3] = {0xAC, 0x33, 0x00};
	HAL_I2C_Master_Transmit(AHT_bus.Instance, 0x70, cmd, 3, 100);
	
	delay_ms(80);
	while(AHT_Read_Status()&0x80==0x80 && cnt)
	{
		delay_ms(5);
		cnt--;
		AHT_Read_Status();
	}
	if(!cnt)
	{return 1;}
	
	uint8_t buf[6] = {0};
	HAL_I2C_Master_Receive(AHT_bus.Instance, 0x71, buf, 6, 100);
	Byte_1th = buf[0];
	Byte_2th = buf[1];
	Byte_3th = buf[2];
	Byte_4th = buf[3];
	Byte_5th = buf[4];
	Byte_6th = buf[5];

	RetuData = (RetuData|Byte_2th)<<8;
	RetuData = (RetuData|Byte_3th)<<8;
	RetuData = (RetuData|Byte_4th);
	RetuData =RetuData >>4;
	*humi = (RetuData * 1000 >> 20);
	*humi /= 10;
	
	RetuData = 0;
	RetuData = (RetuData|(Byte_4th&0x0f))<<8;
	RetuData = (RetuData|Byte_5th)<<8;
	RetuData = (RetuData|Byte_6th);
	RetuData = RetuData&0xfffff;
	*temp = ((RetuData * 2000 >> 20)- 500);
	*temp /= 10;
	
	return 0;
}