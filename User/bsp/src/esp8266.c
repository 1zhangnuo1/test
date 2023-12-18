#include "esp8266.h"

char *str[4] = {"POST /devices/1035410122/datapoints HTTP/1.1",
				"api-key:X1O48XA6TwNx76dYFizHvM3ReHM=",
				"Host:api.heclouds.com",
				""};
char strValue[8] = {0};

// 向onenet发送数据
u8 *esp8266_str_data(char *key, char *value)
{
	u8 i;
	u8 *back;
	char temp[512];
	char temp3[64];		// 长度
	char temp5[128];		// 发送值

	// 拼接post报文
	strcpy(temp5, "{\"datastreams\":[{\"id\":\"");
	strcat(temp5, key);
	strcat(temp5, "\",\"datapoints\":[{\"value\":");
	strcat(temp5, value);
	strcat(temp5, "}]}]}");

	strcpy(temp3, "Content-Length:");
	sprintf(temp, "%d", strlen(temp5) + 1);
	strcat(temp3, temp);

	strcpy(temp, "");
	for (i = 0; i < 3; i++)
	{
		strcat(temp, str[i]);
		strcat(temp, "\r\n");
	}
	strcat(temp, temp3);
	strcat(temp, "\r\n\r\n");
	strcat(temp, temp5);
	strcat(temp, "\r\n");

	back = esp8266_send_data((u8 *)temp, 50);
	// printf("server:%s\r\n", back);
	if (strstr((char *)back, "ERROR"))		//发送失败, 重新初始化,发送
	{
		esp8266_send_cmd("AT+CIPMUX=0", "OK", 50);
		while (esp8266_send_cmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80", "CONNECT", 100));
		esp8266_send_cmd("AT+CIPMODE=1", "OK", 50);
		esp8266_send_cmd("AT+CIPSEND", "OK", 20);
		return esp8266_send_data((u8 *)temp, 50);
	}
	return back;
}

// 向esp8266请求数据
u16 esp8266_get_data(char *vStr)
{
	u8 i;
	u16 value = 0;
	char *back;
	char temp[160] = "GET /devices/1035410122/datastreams/";

	// 拼接请求报文
	strcat(temp, vStr);
	strcat(temp, " HTTP/1.1\r\n");
	for (i = 1; i < 4; i++)
	{
		strcat(temp, str[i]);
		strcat(temp, "\r\n");
	}

	// 发送报文, 获取返回字符串
	back = (char *)esp8266_send_data((u8 *)temp, 50);
	// printf("server:%s\r\n", back);
	
	// 在回送报文中截取出数值
	back = strchr(strstr(back, "\"current_value\":"), ':') + 1;
	while (*back != '}')
	{
		if(*back == '\"'){
			back++;
			continue;
		}
		value = value * 10 + (*back - '0');
		back++;
	}

	return value;
}

//ESP8266模块和PC进入透传模式
void esp8266_start_trans(void)
{
	//设置工作模式 1：station模式   2：AP模式  3：兼容 AP+station模式
	esp8266_send_cmd("AT+CWMODE=1", "OK", 50);

	//让Wifi模块重启的命令
	esp8266_send_cmd("AT+RST", "OK", 50);

	delay_ms(1000); //延时2S等待重启成功
	delay_ms(1000);

	//让模块连接上自己的路由WIFI GOT IP
	while (esp8266_send_cmd("AT+CWJAP=\"dhdh\",\"123456dh\"", "WIFI GOT IP", 500)){
		delay_ms(1);
	};

	//=0：单路连接模式     =1：多路连接模式
	esp8266_send_cmd("AT+CIPMUX=0", "OK", 50);
	delay_ms(1);

	//建立TCP连接  这四项分别代表了 要连接的ID号0~4   连接类型  远程服务器IP地址   远程服务器端口号
	while (esp8266_send_cmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80", "CONNECT", 200)){
		delay_ms(1);
	};

	//产品ID     386234 设备ID   719623723    鉴权信息  202515
	//【Receive from 183.230.40.40 : 1811 】: receivedreceived
	//while(esp8266_send_cmd("*386234#202515#test*","receivedreceived",200));

	//是否开启透传模式  0：表示关闭 1：表示开启透传
	esp8266_send_cmd("AT+CIPMODE=1", "OK", 50);
	delay_ms(1);

	//透传模式下 开始发送数据的指令 这个指令之后就可以直接发数据了
	esp8266_send_cmd("AT+CIPSEND", "OK", 50);
	delay_ms(1);
}

//ESP8266退出透传模式   返回值:0,退出成功;1,退出失败
//配置wifi模块，通过想wifi模块连续发送3个+（每个+号之间 超过10ms,这样认为是连续三次发送+）
u8 esp8266_quit_trans(void)
{
	u8 result = 1;
	u3_printf("+++");
	delay_ms(1000);							   //等待500ms太少 要1000ms才可以退出
	result = esp8266_send_cmd("AT", "OK", 20); //退出透传判断.
	if (result)
		printf("quit_trans failed!");
	else
		printf("quit_trans success!");
	return result;
}

//向ESP8266发送命令
//cmd:发送的命令字符串;ack:期待的应答结果,如果为空,则表示不需要等待应答;waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果);1,发送失败
u8 esp8266_send_cmd(u8 *cmd, u8 *ack, u16 waittime)
{
	u8 res = 0;
	USART3_RX_STA = 0;
	u3_printf("%s\r\n", cmd); //发送命令
	delay_ms(1);
	if (ack && waittime)	  //需要等待应答
	{
		while (--waittime) //等待倒计时
		{
			delay_ms(10);
			if (USART3_RX_STA&0X8000) //接收到期待的应答结果
			{
				
				if (esp8266_check_cmd(ack))
				{
					printf("%s\r\n", (u8 *)USART3_RX_BUF);
					break; //得到有效数据
				}
				USART3_RX_STA = 0;
				//strcpy((char *)USART3_RX_BUF, "");		// 清空接收缓存区
			}
		}
		if (waittime == 0) res = 1;
	}
	return res;
}

//ESP8266发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果;其他,期待应答结果的位置(str的位置)
u8 *esp8266_check_cmd(u8 *str)
{
	char *strx = 0;
	if (USART3_RX_STA & 0X8000) //接收到一次数据了
	{
		USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //添加结束符
		strx = strstr((const char *)USART3_RX_BUF, (const char *)str);
	}
	return (u8 *)strx;
}

//向ESP8266发送数据
//cmd:发送的命令字符串;waittime:等待时间(单位:10ms)
//返回值:发送数据后，服务器的返回验证码
u8 *esp8266_send_data(u8 *cmd, u16 waittime)
{
	char temp[1024];
	char *ack = temp;
	USART3_RX_STA = 0;
	u3_printf("%s", cmd); //发送命令
	delay_ms(1);
	if (waittime)		  //需要等待应答
	{
		while (--waittime) //等待倒计时
		{
			delay_ms(10);
			if (USART3_RX_STA & 0X8000) //接收到期待的应答结果
			{
				USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //添加结束符
				ack = (char *)USART3_RX_BUF;
				USART3_RX_STA = 0;
				break; //得到有效数据
			}
		}
	}
	return (u8 *)ack;
}

// 将数字转为字符串
void numToString(u16 value)
{
	int k = 0, j = 0;
	int num = (int)value;
	char tem[10];
	if (value == 0)
	{
		strValue[0] = '0';
		strValue[1] = '\0';
		return;
	}
	while (num)
	{
		tem[k++] = num % 10 + '0'; //将数字加字符0就变成相应字符
		num /= 10;				   //此时的字符串为逆序
	}
	tem[k] = '\0';
	k = k - 1;
	while (k >= 0)
	{
		strValue[j++] = tem[k--]; //将逆序的字符串转为正序
	}
	strValue[j] = '\0'; //字符串结束标志
}
