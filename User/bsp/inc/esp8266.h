#ifndef __ESP8266
#define __ESP8266

#include <stdlib.h>
#include <string.h>
#include "led.h"
#include "delay.h"

#include "usart.h"
#include "usart3.h"
#include "stm32f10x.h"
#include "sys.h"

extern char strValue[8];
u16 esp8266_get_data(char* vStr);
u8* esp8266_str_data(char* key,char* value);		//ÓÃesp8266Ïòonenet·¢ËÍdata
void esp8266_start_trans(void);
u8 esp8266_quit_trans(void);
u8 esp8266_send_cmd(u8 *cmd,u8 *ack,u16 waittime);
u8* esp8266_check_cmd(u8 *str);
u8* esp8266_send_data(u8 *cmd,u16 waittime);
void numToString(u16 value);
#endif
