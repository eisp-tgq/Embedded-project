//#ifndef __DS18B20_H
//#define __DS18B20_H

//#include<stdint.h>
//#include "gpio.h"
//#include "tim.h"

//#define DS18B20_GPIO_PORT    GPIOA
//#define DS18B20_GPIO_PIN     GPIO_PIN_1

//#define DS18B20_DQ_WRITE(state)    HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, (state) ? GPIO_PIN_SET : GPIO_PIN_RESET)
//#define DS18B20_DQ_READ()          HAL_GPIO_ReadPin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN)

//void DS18B20_Set_IO_Output(void);
//void DS18B20_Set_IO_Input(void);

//uint8_t DS18B20_Init(void);//初始化DS18B20
//short DS18B20_Get_Temp(void);//获取温度
//void DS18B20_Start(void);//开始温度转换
//void DS18B20_Write_Byte(uint8_t dat);//写入一个字节
//uint8_t DS18B20_Read_Byte(void);//读出一个字节
//uint8_t DS18B20_Read_Bit(void);//读出一个位
//uint8_t DS18B20_Check(void);//检测是否存在DS18B20
//void DS18B20_Rst(void);//复位DS18B20    
//#endif
// 







#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f1xx_hal.h"  // 确保包含了正确的HAL库头文件

// 更新为新的GPIO端口和引脚
#define DS18B20_GPIO_PORT    GPIOB
#define DS18B20_GPIO_PIN     GPIO_PIN_6  // 将PIN_1改为PIN_6

#define DS18B20_DQ_WRITE(state)    HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, (state) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define DS18B20_DQ_READ()          HAL_GPIO_ReadPin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN)

void DS18B20_Set_IO_Output(void);
void DS18B20_Set_IO_Input(void);

uint8_t DS18B20_Init(void); // 初始化DS18B20
short DS18B20_Get_Temp(void); // 获取温度
void DS18B20_Start(void); // 开始温度转换
void DS18B20_Write_Byte(uint8_t dat); // 写入一个字节
uint8_t DS18B20_Read_Byte(void); // 读出一个字节
uint8_t DS18B20_Read_Bit(void); // 读出一个位
uint8_t DS18B20_Check(void); // 检测是否存在DS18B20
void DS18B20_Rst(void); // 复位DS18B20
void HAL_Delay_us(uint32_t us);
#endif







