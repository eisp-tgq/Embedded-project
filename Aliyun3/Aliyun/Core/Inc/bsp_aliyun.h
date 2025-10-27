#ifndef __ALIYUN_H__
#define __ALIYUN_H__

#include "main.h"
#include "usart.h"

/*需要修改WiFi账号和密码，必须为电脑热点，且频率仅为2.4GHz*/
#define SSID "abcdef"   //WIFI账号
#define PSWD "12345678"      //密码

//#define SSID "skkk"   //WIFI账号
//#define PSWD "18160505"      //密码

/*需要修改阿里云设备的MQTT连接参数*/
/*注意：逗号之前加\\   a1b4Jc43DrM.Test|securemode=2,signmethod=hmacsha256,timestamp=1719299758491|*/
#define MQTT_clientId "a1EWSWcxfZJ.jiaohua_stm32|securemode=2\\,signmethod=hmacsha256\\,timestamp=1744725682571|"

#define MQTT_username "jiaohua_stm32&a1EWSWcxfZJ"
#define MQTT_passwd	"cead7f6cb059311198811445c7f20eb2f7916e7f693a7c4acde5038353080985"
#define MQTT_mqttHostUrl "a1EWSWcxfZJ.iot-as-mqtt.cn-shanghai.aliyuncs.com"

//post,修改第二个、第三个参数为自己的即可
#define MQTT_post "/sys/a1EWSWcxfZJ/jiaohua_stm32/thing/event/property/post"



void ESP8266_Init(void);
void Aliyun_Connect(void);
void Send_Dat_2_Aliyun(float temp, float ph, float TDS);
void Send_led_dat(int sta_led, int val_led);

#endif /* __ALIYUN_H__ */
