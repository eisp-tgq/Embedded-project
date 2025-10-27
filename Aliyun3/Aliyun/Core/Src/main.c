/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "string.h"
#include "bsp_oled.h"                            //OLED屏幕驱动头文件
#include "bsp_aliyun.h"                          //阿里云头文件
#include "bsp_iic_sensors.h"                     //IIC协议传感器驱动头文件

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t usart2_RX1[512] = {0};                   //串口2接收数据缓冲区,main.h有extern注意看
uint8_t uart2_rec_flag = 0;                      //串口2接收完成中断

extern TIM_HandleTypeDef htim4;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

int find_param(char* str, char* keyword);
void OLED_Show_label(void);
void OLED_Show_Sensordat(float temp, float ph, float TDS);
void OLED_Show_LEDdat(int led_val_dip);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* 时间变量 */
uint16_t year = 2025;  // 初始化为当前年份
uint8_t month = 1;     // 初始化为1月
uint8_t day = 1;       // 初始化为1号
uint8_t hours = 0;
uint8_t minutes = 0;
uint8_t seconds = 0;


/* 访客相关的变量 */
uint8_t visitor_count = 0;
uint8_t last_visitor_seconds = 1;
uint8_t last_visitor_minutes = 1;
uint8_t last_visitor_hours = 1;
uint8_t last_visitor_day = 1;

const uint32_t visitor_timeout = 10;  // 超过10秒认为访客停留过长

/**********语音提醒功能*************/
//语音模块中预设“欢迎光临，请稍等”语句，通过下面命令触发播放
// 发送语音数据（AA 02 00 AC）
void USART2_SendVoiceData(void) {
    uint8_t data[] = {0xAA, 0x02, 0x00, 0xAC};  // 数据字节数组

    // 使用 HAL_UART_Transmit 发送数据
    if (HAL_UART_Transmit(&huart2, data, sizeof(data), 1000) != HAL_OK) {
        // 如果发送失败，可以进行错误处理
        Error_Handler();
    }
}

void OLED_Show_IRStatus(uint8_t status, uint8_t visitor_count) {
    char statusMessage[30];  // 用于存储状态和人数信息

    // 根据红外传感器的状态更新消息
    if (status == 1) {
        // 有人，显示 IR: Someone 和访客人数
        sprintf(statusMessage, "IR: Someone %d", visitor_count);
    } else {
        // 没人，显示 IR: No one
        sprintf(statusMessage, "IR: No one");
    }

    OLED_ShowString(0, 32, (uint8_t*)statusMessage, 16, 1);  // 第三行：显示红外传感器状态和人数

    OLED_Refresh();  // 刷新屏幕显示
}


/* 外部中断服务例程 */
void EXTI0_IRQHandler(void) {
    // 检查中断源
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);  // 清除中断标志

        // 访客到来时的处理
        visitor_count++;
        if (visitor_count > 0) {
            OLED_Show_IRStatus(1, visitor_count);  // 更新访客计数
        }
				
				// 记录访客到达时的时、分、秒、天
        last_visitor_seconds = seconds;
        last_visitor_minutes = minutes;
        last_visitor_hours = hours;
        last_visitor_day = day;
        USART2_SendVoiceData();  // 发送语音提醒
    }
}

// 计算访客停留时间
void Check_Visitor_Stay(void) {
    // 如果检测到访客，并且访客停留时间超过设定时间（5秒）
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET) {
        // 计算当前时间与访客到达时的时间差
        uint32_t stay_time_seconds = 0;

        // 计算总秒数
        uint32_t current_time_in_seconds = seconds + minutes * 60 + hours * 3600 + day * 86400;
        uint32_t visitor_arrival_time_in_seconds = last_visitor_seconds + last_visitor_minutes * 60 + last_visitor_hours * 3600 + last_visitor_day * 86400;

        if (current_time_in_seconds >= visitor_arrival_time_in_seconds) {
            stay_time_seconds = current_time_in_seconds - visitor_arrival_time_in_seconds;
        } else {
            stay_time_seconds = (UINT32_MAX - visitor_arrival_time_in_seconds) + current_time_in_seconds + 1; // 处理溢出情况
        }

        // 判断停留时间是否超过设定的阈值（如5秒）
        if (stay_time_seconds >= visitor_timeout) {
            // 如果超过设定时间，启动蜂鸣器报警
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);  // 启动蜂鸣器
								HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
								HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        }
    }
}

// 计算星期几的函数（0:星期日, 1:星期一, ... 6:星期六）
int get_weekday(uint16_t year, uint8_t month, uint8_t day) {
    if (month <= 2) {
        month += 12;
        year--;
    }
    int w = (day + 2 * month + (3 * (month + 1)) / 5 + year + (year / 4) - (year / 100) + (year / 400)) % 7;
    return w;  // 返回星期几（0:星期日, 1:星期一, ..., 6:星期六）
}


/* 判断闰年的函数 */
int is_leap_year(uint16_t year) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return 1;  // 是闰年
    }
    return 0;  // 不是闰年
}

/* 获取某月的天数（考虑闰年） */
int get_days_in_month(uint8_t month, uint16_t year) {
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};  // 非闰年的月份天数
    if (month == 2 && is_leap_year(year)) {  // 2月的天数需要特殊处理
        return 29;  // 闰年2月有29天
    }
    return days_in_month[month - 1];  // 返回对应月份的天数
}

void OLED_Show_Time(uint16_t year, uint8_t month, uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds) {
    char timeStr1[20];  // 用于显示年月日和小时
    char timeStr2[20];  // 用于显示分钟、秒和星期几

    // 计算星期几
    int weekday = get_weekday(year, month, day);
    const char* weekStr[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    // 格式化字符串：第一行显示 年月日 时
    sprintf(timeStr1, "%04d-%02d-%02d %02d", year, month, day, hours);

    // 格式化字符串：第二行显示 分秒 星期几
    sprintf(timeStr2, "%02d:%02d %s", minutes, seconds, weekStr[weekday]);

    // 清除并更新第一行（年月日时）
    OLED_ShowString(0, 0, (uint8_t*)timeStr1, 16, 1);  // 更新第一行内容
    OLED_ShowString(0, 16, (uint8_t*)timeStr2, 16, 1);  // 更新第二行内容

    OLED_Refresh();  // 如果需要刷新屏幕，可以调用 OLED_Refresh()
}


/* 设置项的索引 */
#define SETTING_YEAR   0
#define SETTING_MONTH  1
#define SETTING_DAY    2
#define SETTING_HOUR   3
#define SETTING_MINUTE 4

/* 当前选中的设置项 */
int current_setting = SETTING_YEAR;  // 默认选择“年”

/* 消抖处理 */
volatile uint32_t last_debounce_time = 0;
const uint32_t debounce_delay = 10;  // 消抖时间，单位毫秒

/* 显示当前选项 */
void OLED_Show_CurrentSetting() {
    char timeStr[20];  // YYYY-MM-DD HH:MM格式

    // 根据当前选项设置不同的格式显示
    switch (current_setting) {
        case SETTING_YEAR:
            sprintf(timeStr, "Year: %04d", year);
            break;
        case SETTING_MONTH:
            sprintf(timeStr, "Month: %02d", month);
            break;
        case SETTING_DAY:
            sprintf(timeStr, "Day: %02d", day);
            break;
        case SETTING_HOUR:
            sprintf(timeStr, "Hour: %02d", hours);
            break;
        case SETTING_MINUTE:
            sprintf(timeStr, "Minute: %02d", minutes);
            break;
    }

    OLED_Clear();  // 清除OLED屏幕内容
    OLED_ShowString(0, 0, (uint8_t*)timeStr, 16, 1);  // 显示当前选中的设置
    OLED_Refresh();  // 刷新屏幕
}

/* PA4中断处理函数：切换设置项 */
void EXTI4_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);  // 清除中断标志位

        // 检查消抖时间
        if (HAL_GetTick() - last_debounce_time >= debounce_delay) {
            // 切换设置项
            current_setting++;
            if (current_setting > SETTING_MINUTE) {
                current_setting = SETTING_YEAR;  // 循环回到“年”
            }

            OLED_Show_CurrentSetting();  // 显示当前选项
            last_debounce_time = HAL_GetTick();  // 更新消抖时间
        }
    }
}


/* PA7中断处理函数：确认设置 */
void EXTI9_5_IRQHandler(void) {
	if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);  // 清除中断标志位

        // 检查消抖时间
        if (HAL_GetTick() - last_debounce_time >= debounce_delay) {
            // 增加当前选中的设置项的值
            switch (current_setting) {
                case SETTING_YEAR:
                    year++;
                    if (year > 9999) year = 1900;  // 最大值9999，最小值1900
                    break;
                case SETTING_MONTH:
                    month++;
                    if (month > 12) month = 1;  // 最大值12，最小值1
                    break;
                case SETTING_DAY:
                    if (day < get_days_in_month(month, year)) {
                        day++;
                    } else {
                        day = 1;  // 如果超过了该月天数，则回到1号
                    }
                    break;
                case SETTING_HOUR:
                    hours++;
                    if (hours >= 24) hours = 0;  // 最大值23，最小值0
                    break;
                case SETTING_MINUTE:
                    minutes++;
                    if (minutes >= 60) minutes = 0;  // 最大值59，最小值0
                    break;
            }
            OLED_Show_CurrentSetting();  // 显示当前选项
            last_debounce_time = HAL_GetTick();  // 更新消抖时间
        }
    }
	else if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);  // 清除中断标志位

        // 检查消抖时间
        if (HAL_GetTick() - last_debounce_time >= debounce_delay) {
            // 减少当前选中的设置项的值
            switch (current_setting) {
                case SETTING_YEAR:
                    if (year <= 1900) year = 9999;  // 如果年小于1900，则跳到最大值9999
                    else year--;
                    break;
                case SETTING_MONTH:
                    if (month <= 1) month = 12;  // 如果月小于1，则跳到最大值12
                    else month--;
                    break;
                case SETTING_DAY:
                    if (day > 1) {
                        day--;
                    } else {
                        day = get_days_in_month(month, year);  // 如果已经是1号，跳转到该月的最大天数
                    }
                    break;
                case SETTING_HOUR:
                    if (hours <= 0) hours = 23;  // 如果小时为0，则跳到最大值23
                    else hours--;
                    break;
                case SETTING_MINUTE:
                    if (minutes <= 0) minutes = 59;  // 如果分钟为0，则跳到最大值59
                    else minutes--;
                    break;
            }
            OLED_Show_CurrentSetting();  // 显示当前选项
            last_debounce_time = HAL_GetTick();  // 更新消抖时间
        }
    }
	else if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);  // 清除中断标志位

        // 确认当前设置项，执行相关操作（如保存到存储器或设置硬件）
        switch (current_setting) {
            case SETTING_YEAR:
                // 确认年份设置
                break;
            case SETTING_MONTH:
                // 确认月份设置
                break;
            case SETTING_DAY:
                // 确认日期设置
                break;
            case SETTING_HOUR:
                // 确认小时设置
                break;
            case SETTING_MINUTE:
                // 确认分钟设置
                break;
        }
        last_debounce_time = HAL_GetTick();  // 更新消抖时间
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4) {
        seconds++;
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                if (hours >= 24) {
                    hours = 0;
                    day++;  // 每天更新日期
                    if (day > get_days_in_month(month, year)) {  // 如果日期超过了当月最大天数
                        day = 1;  // 重置为1号
                        month++;   // 月份加1
                        if (month > 12) {  // 如果月份超过12月
                            month = 1;   // 重置为1月
                            year++;      // 年份加1
                        }
                    }
                }
            }
        }

        // 在OLED屏幕上显示当前时间和日期
        OLED_Show_Time(year, month, day, hours, minutes, seconds);
    }
}

uint8_t owner_status = 0;  // 0表示没有人，1表示有主人

void OLED_Show_OwnerStatus(void) {
    // 显示简短的消息
    char statusMessage[] = "Home no owner";

    // 在第四行显示消息
    OLED_ShowString(0, 48, (uint8_t*)statusMessage, 16, 1);  // 第四行：显示消息

    OLED_Refresh();  // 刷新屏幕显示
}

void EXTI1_IRQHandler(void) {
    // 检查中断源
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_1) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);  // 清除中断标志

        // 切换房屋状态（有/没有主人）
        if (owner_status == 0) {
            owner_status = 1;  // 房屋有主人
        } else {
            owner_status = 0;  // 房屋没有主人
        }

        // 根据房屋状态更新显示
        if (owner_status == 1) {
            OLED_Show_OwnerStatus();  // 显示房屋有主人的状态
        } else {
            // 没有主人时，显示空格来清除该行内容
            OLED_ShowString(0, 48, (uint8_t*)"                        ", 16, 1);  // 用空格清除显示
            OLED_Refresh();
        }
				
				
				/********清除陌生人停留报警状态**********/
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    }
}




/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  uint16_t post_time_count = 0;                  //定时任务变量
	
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
	MX_KEY_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
	
	HAL_TIM_Base_Init(&htim4);  // 重新初始化定时器
	HAL_TIM_Base_Start_IT(&htim4);  // 启动定时器中断
  /* USER CODE BEGIN 2 */
	
	OLED_Init();
	OLED_ColorTurn(0);                             //0正常显示，1 反色显示
  OLED_DisplayTurn(0);                           //0正常显示 1 屏幕翻转显示
	OLED_ShowString(28,16,(uint8_t *)"Welcome to",16,1);
	OLED_ShowString(28,32,(uint8_t *)"This System",16,1);
	OLED_Refresh();                                //刷新屏幕内容
	
//	HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_4);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2,usart2_RX1,sizeof(usart2_RX1)); //第三个参数是指最多能接收的字节数量
	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);                    //关闭DMA过半中断

	OLED_Clear();
//	OLED_Show_label();
	
	HAL_Delay(1000);
	

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    Check_Visitor_Stay(); // 检查访客停留时间
		
    if(++post_time_count >= 2000)                //定时上传数据，更改上传频率可适当改小此阈值，
		{
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);  //数据上传指示灯亮
			post_time_count = 0;
			//USART2_SendVoiceData();
			//OLED_Show_OwnerStatus();
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);    //上传结束，熄灭
		}
				
		
		HAL_Delay(1);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart->Instance == USART2)
	{
		HAL_UART_Transmit(&huart1,usart2_RX1,Size,10);                        //串口1打印接受的数据
		
		char *pos_test = strstr((char*)usart2_RX1, "+MQTTSUBRECV");           //阿里云下发指令由+MQTTSUBRECV开头
		if(pos_test != NULL){
			uart2_rec_flag = 1;
		}
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2,usart2_RX1,sizeof(usart2_RX1));  //重新打开DMA接收
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);                     //关闭DMA过半中断
	}
}

/*
 * @brief find_param
 * @retval 从字符串数据中查找关键词
 */
int find_param(char* str, char* keyword) 
{
	char* pos = strstr(str, keyword);
	int num = 0;
	
	if (pos != NULL)
	{
		pos += strlen(keyword);
		while (*pos != ':'){ 
			pos++; //寻找冒号,定位到参数起始位置
		}
		
		pos++;  //跳过冒号
		char param_value[100];
		int i = 0;
		
		while(*pos != ',' && *pos != '}'){ 
			param_value[i++] = *pos++; //提取参数,直到遇到逗号或者右花括号
		}
		param_value[i] = '\0';
		num = atoi(param_value);
	}
	
	return num;
}


void OLED_Show_Sensordat(float temp, float ph, float TDS)
{
    char temp_str[10];      // 用于存储温度的字符串
    char ph_str[10];        // 用于存储pH值的字符串
    char turbidity_str[10]; // 用于存储水浊度的字符串
    char ec_str[10];        // 用于存储电导率的字符串

    // 处理温度数据
    sprintf(temp_str, "%.1f", temp); // 将温度数据转换为字符串，保留1位小数
		OLED_ShowString(0, 0, (uint8_t*)"Temp", 16, 1);  // 显示 "Temp"
    OLED_ShowString(41, 0, (uint8_t*)temp_str, 16, 1);  // 显示温度字符串

    // 处理PH值
    sprintf(ph_str, "%.1f", ph); // 将PH值数据转换为字符串，保留1位小数
		OLED_ShowString(0, 17, (uint8_t*)"pH", 16, 1);  // 显示 "pH"
    OLED_ShowString(41, 17, (uint8_t*)ph_str, 16, 1); // 显示pH值字符串

    // 处理电导率数据
    sprintf(ec_str, "%.1f", TDS); // 将TDS数据转换为字符串，保留1位小数
		OLED_ShowString(0, 33, (uint8_t*)"TDS", 16, 1);  // 显示 "EC"
    OLED_ShowString(41, 33, (uint8_t*)ec_str, 16, 1);  // 显示电导率字符串

    // 刷新屏幕
    OLED_Refresh();
}


void OLED_Show_label(void)
{
	OLED_ShowChinese(0,0,0,16,1);                     //显示“温”
	OLED_ShowChinese(17,0,1,16,1);                    //显示“度”
	OLED_ShowChinese(33,0,2,16,1);                    //显示“:”
	
	OLED_ShowChinese(0,17,4,16,1);                    //显示“湿”
	OLED_ShowChinese(17,17,1,16,1);                   //显示“度”
	OLED_ShowChinese(33,17,2,16,1);                   //显示“:”
	
	OLED_ShowChinese(0,33,5,16,1);                    //显示“亮”
	OLED_ShowChinese(17,33,1,16,1);                   //显示“度”
	OLED_ShowChinese(33,33,2,16,1);                   //显示“:”
	
	OLED_ShowString(0,49,(uint8_t *)"LED:",16,1);		
	OLED_Refresh();                                   //刷新屏幕内容
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
