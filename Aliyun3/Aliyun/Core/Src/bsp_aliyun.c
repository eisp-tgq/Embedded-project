#include "bsp_aliyun.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "bsp_oled.h"

uint8_t usart2_tx_buff[512]; //发送缓存区

void Usart2_Send_Data(uint8_t data)
{
	while(!(USART2->SR & 0x01<<7));
	USART2->DR =data;
}

void Usart2_Send_Str(uint8_t *buf)
{
	while(*buf != 0)
	{
		Usart2_Send_Data(*buf);
		buf++;
	}
}

uint8_t FindStr(char* dest,char* src,uint16_t retry_nms)
{
    retry_nms/=10;                   //超时时间

    while((strstr(dest,src)==0) && retry_nms--)//等待串口接收完毕或超时退出
    {		
			HAL_Delay(10);
    }
	retry_nms+=1;
    if(retry_nms)
	 {
		
		 return 1;
	 }

	return 0; 
}

static uint8_t ConnectAP(char* ssid,char* pswd)
{
	uint8_t cnt=5;
	while(cnt--)
	{
		memset(usart2_RX1,0,512);
		Usart2_Send_Str((uint8_t *)"AT+CWMODE=1\r\n");              //设置为STATION模式	
		if(FindStr((char*)usart2_RX1,"OK",20000) != 0)
		{
			break;
		}             		
	}
	if(cnt == 0) 
		return 0;

	cnt=2;
	while(cnt--)
	{
		memset(usart2_RX1,0,512);                            //清空发送缓冲
		memset(usart2_tx_buff,0,512);                            //清空接收缓冲
		sprintf((char*)usart2_tx_buff,"AT+CWJAP=\"%s\",\"%s\"\r\n",ssid,pswd);//连接目标AP
		Usart2_Send_Str(usart2_tx_buff);	
		if(FindStr((char*)usart2_RX1,"OK",8000)!=0)                      //连接成功且分配到IP
		{
			return 1;
		}
	}
	return 0;
}

void ESP8266_Init(void)
{
	uint8_t flag=0;
	
	memset(usart2_RX1,0,512);
	Usart2_Send_Str((uint8_t *)"AT\r\n"); //检测ESP8266是否存在
	if(FindStr((char*)usart2_RX1,"OK",2000) != 0){
		printf("Detect ESP8266\r\n");
	}
	else{
		if(FindStr((char*)usart2_RX1,"K",2000) != 0){
			printf("Detect ESP8266\r\n");
		}
		else{
			printf("Detect ESP8266 Fail\r\n");
		}
	}
	HAL_Delay(100);
	
	flag = ConnectAP(SSID,PSWD);
	OLED_Clear();                                              //清除屏幕内容
	if(flag==0){
		printf("WIFI连接失败\r\n");
		OLED_ShowString(8,16,(uint8_t *)"WIFI connection",16,1); //拆成两行显示 
		OLED_ShowString(40,32,(uint8_t *)"failed",16,1);	
	}
	else{
		printf("WIFI连接成功\r\n");
		OLED_ShowString(36,16,(uint8_t *)"WiFi is",16,1);
		OLED_ShowString(28,32,(uint8_t *)"connected",16,1);
	}
	OLED_Refresh();                                          //刷新屏幕内容
}

void Aliyun_Connect(void)
{
	char texta[488];
	
	memset(usart2_RX1,0,512); 	              //清空接收缓冲
	memset(usart2_tx_buff,0,512); 	              //清空发送缓冲
	sprintf((char*)usart2_tx_buff,"AT+MQTTUSERCFG=0,1,\"NULL\",\"");
	sprintf(texta,"%s\",\"%s\",0,0,\"\"\r\n",MQTT_username,MQTT_passwd);
	strcat((char*)usart2_tx_buff,texta);           
	Usart2_Send_Str((uint8_t *)usart2_tx_buff);   //发送AT指令
	HAL_Delay(3000);
	
	memset(usart2_RX1,0,512); 	              //清空接收缓冲
	memset(usart2_tx_buff,0,512); 	              //清空发送缓冲
	sprintf((char*)usart2_tx_buff,"AT+MQTTCLIENTID=0,\"%s\"\r\n",MQTT_clientId);         
	Usart2_Send_Str((uint8_t *)usart2_tx_buff);   //发送AT指令
	HAL_Delay(3000);
	
	memset(usart2_RX1,0,512); 	              //清空接收缓冲
	memset(usart2_tx_buff,0,512); 	              //清空发送缓冲
	sprintf((char*)usart2_tx_buff,"AT+MQTTCONN=0,\"%s\",1883,1\r\n",MQTT_mqttHostUrl);         
	Usart2_Send_Str((uint8_t *)usart2_tx_buff);   //发送AT指令
	HAL_Delay(3000);
}

//void Send_Dat_2_Aliyun(float temp, float humi, float pH)
//{
//	char text[48];
//	
//	memset(usart2_tx_buff,0,512); 	                               //清空发送缓冲
//	sprintf((char*)usart2_tx_buff,"AT+MQTTPUB=0,\"%s\",\"{\\\"params\\\":{", MQTT_post);
//	sprintf(text,"\\\"temp\\\":%.1f\\,",temp);   //temp是阿里云的温度标识符
//	strcat((char*)usart2_tx_buff,text);                            //将传感器数据整合进字符串
//	sprintf(text,"\\\"humi\\\":%.1f\\,",humi);  //humi是阿里云的湿度标识符
//	strcat((char*)usart2_tx_buff,text);
//	sprintf(text,"\\\"ph\\\":%.1f",pH);                //light是阿里云的湿度标识符
//	strcat((char*)usart2_tx_buff,text);
//	strcat((char*)usart2_tx_buff,"}}\",1,0\r\n");
//	Usart2_Send_Str((uint8_t *)usart2_tx_buff);
//}

void Send_Dat_2_Aliyun(float temp, float ph, float TDS)
{
    char text[64];
    
    memset(usart2_tx_buff, 0, 512); // 清空发送缓冲区

    // 构建MQTT消息头
    sprintf((char*)usart2_tx_buff, "AT+MQTTPUB=0,\"%s\",\"{\\\"params\\\":{", MQTT_post);

    // 添加温度数据
    sprintf(text, "\\\"temp\\\":%.1f\\,", temp); // temp是阿里云的温度标识符
    strcat((char*)usart2_tx_buff, text); // 将温度数据整合进字符串

    // 添加ph数据
    sprintf(text, "\\\"ph\\\":%.1f\\,", ph); // humi是阿里云的湿度标识符
    strcat((char*)usart2_tx_buff, text); // 将湿度数据整合进字符串

     // 添加TDS数据
    sprintf(text, "\\\"tds\\\":%.1f", TDS); // tds是电导率标识符
    strcat((char*)usart2_tx_buff, text); // 将电导率数据整合进字符串

    // 结束消息
    strcat((char*)usart2_tx_buff, "}}\",1,0\r\n");

    // 发送数据
    Usart2_Send_Str((uint8_t *)usart2_tx_buff);
}

void Send_led_dat(int sta_led, int val_led)
{
	char text[48];
		
	memset(usart2_tx_buff,0,512); 	                //清空发送缓冲
	sprintf((char*)usart2_tx_buff,"AT+MQTTPUB=0,\"%s\",\"{\\\"params\\\":{", MQTT_post);
	sprintf(text,"\\\"LEDSta\\\":%d\\,",sta_led);   //LEDSta是阿里云的LED开关标识符
	strcat((char*)usart2_tx_buff,text);                            
	sprintf(text,"\\\"LEDVal\\\":%d",val_led);      //LEDVal是阿里云的LED亮度标识符
	strcat((char*)usart2_tx_buff,text);
	strcat((char*)usart2_tx_buff,"}}\",1,0\r\n");
	Usart2_Send_Str((uint8_t *)usart2_tx_buff);
}
