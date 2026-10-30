#include "lcd_init.h"
#include "delay.h"
#include "spi.h"
#include "tim.h"

/******************************************************************************
      ����˵����LCD�˿ڳ�ʼ��
      ������ݣ���
      ����ֵ��  ��
******************************************************************************/
void LCD_GPIO_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure = {0};
	
 	__HAL_RCC_GPIOA_CLK_ENABLE();
 	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitStructure.Pin = RES_PIN|DC_PIN;	 
 	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 		 //推挽输出
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;//速度50MHz
 	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);	  //初始化GPIOB
 	HAL_GPIO_WritePin(GPIOB, RES_PIN|DC_PIN, GPIO_PIN_SET);

	GPIO_InitStructure.Pin = CS_PIN;
 	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
 	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
 	HAL_GPIO_WritePin(GPIOA, CS_PIN, GPIO_PIN_SET);
}


void LCD_WR_REG(u8 dat)
{
	LCD_DC_Clr();
	LCD_CS_Clr();
	HAL_SPI_Transmit(&hspi1, &dat, 1, 100);
	LCD_CS_Set();
}

void LCD_WR_DATA8(u8 dat)
{
	LCD_DC_Set();
	LCD_CS_Clr();
	HAL_SPI_Transmit(&hspi1, &dat, 1, 100);
	LCD_CS_Set();
}

void LCD_WR_DATA(u16 dat)
{
	uint8_t temp[2];
	temp[0] = (dat >> 8) & 0xff;
	temp[1] = dat & 0xff;
	LCD_DC_Set();
	LCD_CS_Clr();
	HAL_SPI_Transmit(&hspi1, temp, 2, 100);
	LCD_CS_Set();
}


/******************************************************************************
      ����˵����������ʼ�ͽ�����ַ
      ������ݣ�x1,x2 �����е���ʼ�ͽ�����ַ
                y1,y2 �����е���ʼ�ͽ�����ַ
      ����ֵ��  ��
******************************************************************************/
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
	LCD_WR_REG(0x2a);//�е�ַ����
	LCD_WR_DATA(x1);
	LCD_WR_DATA(x2);
	LCD_WR_REG(0x2b);//�е�ַ����
	LCD_WR_DATA(y1);
	LCD_WR_DATA(y2);
	LCD_WR_REG(0x2c);//������д
}


/******************************************************************************
      ����˵����LCD���ڱ���
      ������ݣ�dc,ռ�ձ�,5%~100%
      ����ֵ��  ��
******************************************************************************/
void LCD_Set_Light(uint8_t dc)
{
	if(dc>=5 && dc<=100)
		__HAL_TIM_SetCompare(&htim2,TIM_CHANNEL_1,dc*PWM_PERIOD/100);
}


/******************************************************************************
      ����˵����LCD�رձ���
      ������ݣ���
      ����ֵ��  ��
******************************************************************************/
void LCD_Close_Light(void)
{
	__HAL_TIM_SetCompare(&htim2,TIM_CHANNEL_1,0);
	HAL_GPIO_WritePin(BLK_PORT,BLK_PIN,GPIO_PIN_RESET);
}


/******************************************************************************
      ����˵����LCD��������
      ������ݣ���
      ����ֵ��  ��
******************************************************************************/
void LCD_Open_Light(void)
{
	HAL_GPIO_WritePin(BLK_PORT,BLK_PIN,GPIO_PIN_SET);
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
}
/******************************************************************************
      ����˵����ST7789 SLEEP IN
      ������ݣ���
      ����ֵ��  ��
******************************************************************************/
void LCD_ST7789_SleepIn(void)
{
	LCD_WR_REG(0x10);
	delay_ms(100);
}


/******************************************************************************
      ����˵����ST7789 SLEEP OUT
      ������ݣ���
      ����ֵ��  ��
******************************************************************************/
void LCD_ST7789_SleepOut(void)
{
	LCD_WR_REG(0x11);
	delay_ms(100);
}


/******************************************************************************
      ����˵����LCD��ʼ��
      ������ݣ���
      ����ֵ��  ��
******************************************************************************/
void LCD_Init(void)
{
	LCD_GPIO_Init();

	LCD_RES_Clr();
	delay_ms(10);
	LCD_RES_Set();
	delay_ms(20);

	LCD_WR_REG(0x36);
	if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x00);
	else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC0);
	else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x70);
	else LCD_WR_DATA8(0xA0);

	LCD_WR_REG(0x3A);
	LCD_WR_DATA8(0x55);

	LCD_WR_REG(0xB2);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x33);

	LCD_WR_REG(0xB7);
	LCD_WR_DATA8(0x35);

	LCD_WR_REG(0xBB);
	LCD_WR_DATA8(0x19);

	LCD_WR_REG(0xC0);
	LCD_WR_DATA8(0x2C);

	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0x01);

	LCD_WR_REG(0xC3);
	LCD_WR_DATA8(0x12);

	LCD_WR_REG(0xC4);
	LCD_WR_DATA8(0x20);

	LCD_WR_REG(0xC6);
	LCD_WR_DATA8(0x0F);

	LCD_WR_REG(0xD0);
	LCD_WR_DATA8(0xA4);
	LCD_WR_DATA8(0xA1);

	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x11);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x2B);
	LCD_WR_DATA8(0x3F);
	LCD_WR_DATA8(0x54);
	LCD_WR_DATA8(0x4C);
	LCD_WR_DATA8(0x18);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x0B);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x23);

	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x11);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x2C);
	LCD_WR_DATA8(0x3F);
	LCD_WR_DATA8(0x44);
	LCD_WR_DATA8(0x51);
	LCD_WR_DATA8(0x2F);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x20);
	LCD_WR_DATA8(0x23);

	LCD_WR_REG(0x21);

	LCD_WR_REG(0x11);
	delay_ms(120);

	LCD_WR_REG(0x13);

	LCD_WR_REG(0x29);
	delay_ms(50);
}