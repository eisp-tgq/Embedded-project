#include "bsp_iic_sensors.h"

/**************************AHT10温湿度传感器部分************************************************/
/*
 * 函数功能：读取温湿度数值
 * 返回参数：指针函数,返回的第一个值为温度,第二个参数为湿度
 */
float* Get_AHT10_TH(void)
{
	uint8_t Send_Data[3]={0xAC, 0x33, 0x00};
	uint8_t Rece_Data[6]={0};
	int Data_Process[3] ={0};
	static float ah10_data[2] ={0};                                 //必须静态数组可以返回

	HAL_I2C_Master_Transmit(&hi2c2,AHT10_W,Send_Data,3,10);         //发送测量命令
	HAL_Delay(100);                                                 //延时至少75ms
	HAL_I2C_Master_Receive(&hi2c2,AHT10_R,Rece_Data,6,10);          //接收信息

	Data_Process[0] = Rece_Data[0];
	Data_Process[1] = (Rece_Data[1]<<12) | (Rece_Data[2]<<4) | (Rece_Data[3]>>4);
	Data_Process[2] = ((Rece_Data[3]&0x0f)<<16) | (Rece_Data[4]<<8) | Rece_Data[5];

	ah10_data[0] = (Data_Process[2] * 200.0) / 1024.0 / 1024 - 50;  //温度信息
	ah10_data[1] = (Data_Process[1] * 100.0) / 1024.0 / 1024;       //湿度信息

	return ah10_data;
}

/**************************BH1750光照传感器部分*************************************************/
float Get_BH1750_LUX(BH1750_MODE cmd)
{
	uint8_t dat[2]={0};
	uint16_t temp_dat = 0;
	float temp_data[3] = {0.0};
	float aver_data = 0.0;

	for(uint8_t i=0; i<3; i++)
	{
		HAL_I2C_Master_Transmit(&hi2c2,BH1750_ADDR_WRITE,(uint8_t*)&cmd,1,10);
		HAL_Delay(200);  //至少测量120ms
		HAL_I2C_Master_Receive(&hi2c2,BH1750_ADDR_READ,dat,2,10);
		temp_dat = (dat[0] << 8) | dat[1];
		temp_data[i] = (float)(temp_dat/1.2);
		aver_data += temp_data[i];
	}

	return aver_data/3;
}
