#include "esp8266.h"

char *str[4] = {"POST /devices/1035410122/datapoints HTTP/1.1",
				"api-key:X1O48XA6TwNx76dYFizHvM3ReHM=",
				"Host:api.heclouds.com",
				""};
char strValue[8] = {0};

// ��onenet��������
u8 *esp8266_str_data(char *key, char *value)
{
	u8 i;
	u8 *back;
	char temp[512];
	char temp3[64];		// ����
	char temp5[128];		// ����ֵ

	// ƴ��post����
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
	if (strstr((char *)back, "ERROR"))		//����ʧ��, ���³�ʼ��,����
	{
		esp8266_send_cmd("AT+CIPMUX=0", "OK", 50);
		while (esp8266_send_cmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80", "CONNECT", 100));
		esp8266_send_cmd("AT+CIPMODE=1", "OK", 50);
		esp8266_send_cmd("AT+CIPSEND", "OK", 20);
		return esp8266_send_data((u8 *)temp, 50);
	}
	return back;
}

// ��esp8266��������
u16 esp8266_get_data(char *vStr)
{
	u8 i;
	u16 value = 0;
	char *back;
	char temp[160] = "GET /devices/1035410122/datastreams/";

	// ƴ��������
	strcat(temp, vStr);
	strcat(temp, " HTTP/1.1\r\n");
	for (i = 1; i < 4; i++)
	{
		strcat(temp, str[i]);
		strcat(temp, "\r\n");
	}

	// ���ͱ���, ��ȡ�����ַ���
	back = (char *)esp8266_send_data((u8 *)temp, 50);
	// printf("server:%s\r\n", back);
	
	// �ڻ��ͱ����н�ȡ����ֵ
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

//ESP8266ģ���PC����͸��ģʽ
void esp8266_start_trans(void)
{
	//���ù���ģʽ 1��stationģʽ   2��APģʽ  3������ AP+stationģʽ
	esp8266_send_cmd("AT+CWMODE=1", "OK", 50);

	//��Wifiģ������������
	esp8266_send_cmd("AT+RST", "OK", 50);

	delay_ms(1000); //��ʱ2S�ȴ������ɹ�
	delay_ms(1000);

	//��ģ���������Լ���·��WIFI GOT IP
	while (esp8266_send_cmd("AT+CWJAP=\"dhdh\",\"123456dh\"", "WIFI GOT IP", 500)){
		delay_ms(1);
	};

	//=0����·����ģʽ     =1����·����ģʽ
	esp8266_send_cmd("AT+CIPMUX=0", "OK", 50);
	delay_ms(1);

	//����TCP����  ������ֱ������ Ҫ���ӵ�ID��0~4   ��������  Զ�̷�����IP��ַ   Զ�̷������˿ں�
	while (esp8266_send_cmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80", "CONNECT", 200)){
		delay_ms(1);
	};

	//��ƷID     386234 �豸ID   719623723    ��Ȩ��Ϣ  202515
	//��Receive from 183.230.40.40 : 1811 ��: receivedreceived
	//while(esp8266_send_cmd("*386234#202515#test*","receivedreceived",200));

	//�Ƿ���͸��ģʽ  0����ʾ�ر� 1����ʾ����͸��
	esp8266_send_cmd("AT+CIPMODE=1", "OK", 50);
	delay_ms(1);

	//͸��ģʽ�� ��ʼ�������ݵ�ָ�� ���ָ��֮��Ϳ���ֱ�ӷ�������
	esp8266_send_cmd("AT+CIPSEND", "OK", 50);
	delay_ms(1);
}

//ESP8266�˳�͸��ģʽ   ����ֵ:0,�˳��ɹ�;1,�˳�ʧ��
//����wifiģ�飬ͨ����wifiģ����������3��+��ÿ��+��֮�� ����10ms,������Ϊ���������η���+��
u8 esp8266_quit_trans(void)
{
	u8 result = 1;
	u3_printf("+++");
	delay_ms(1000);							   //�ȴ�500ms̫�� Ҫ1000ms�ſ����˳�
	result = esp8266_send_cmd("AT", "OK", 20); //�˳�͸���ж�.
	if (result)
		printf("quit_trans failed!");
	else
		printf("quit_trans success!");
	return result;
}

//��ESP8266��������
//cmd:���͵������ַ���;ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��;waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����);1,����ʧ��
u8 esp8266_send_cmd(u8 *cmd, u8 *ack, u16 waittime)
{
	u8 res = 0;
	USART3_RX_STA = 0;
	u3_printf("%s\r\n", cmd); //��������
	delay_ms(1);
	if (ack && waittime)	  //��Ҫ�ȴ�Ӧ��
	{
		while (--waittime) //�ȴ�����ʱ
		{
			delay_ms(10);
			if (USART3_RX_STA&0X8000) //���յ��ڴ���Ӧ����
			{
				
				if (esp8266_check_cmd(ack))
				{
					printf("%s\r\n", (u8 *)USART3_RX_BUF);
					break; //�õ���Ч����
				}
				USART3_RX_STA = 0;
				//strcpy((char *)USART3_RX_BUF, "");		// ��ս��ջ�����
			}
		}
		if (waittime == 0) res = 1;
	}
	return res;
}

//ESP8266���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����;����,�ڴ�Ӧ������λ��(str��λ��)
u8 *esp8266_check_cmd(u8 *str)
{
	char *strx = 0;
	if (USART3_RX_STA & 0X8000) //���յ�һ��������
	{
		USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //��ӽ�����
		strx = strstr((const char *)USART3_RX_BUF, (const char *)str);
	}
	return (u8 *)strx;
}

//��ESP8266��������
//cmd:���͵������ַ���;waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:�������ݺ󣬷������ķ�����֤��
u8 *esp8266_send_data(u8 *cmd, u16 waittime)
{
	char temp[1024];
	char *ack = temp;
	USART3_RX_STA = 0;
	u3_printf("%s", cmd); //��������
	delay_ms(1);
	if (waittime)		  //��Ҫ�ȴ�Ӧ��
	{
		while (--waittime) //�ȴ�����ʱ
		{
			delay_ms(10);
			if (USART3_RX_STA & 0X8000) //���յ��ڴ���Ӧ����
			{
				USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //��ӽ�����
				ack = (char *)USART3_RX_BUF;
				USART3_RX_STA = 0;
				break; //�õ���Ч����
			}
		}
	}
	return (u8 *)ack;
}

// ������תΪ�ַ���
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
		tem[k++] = num % 10 + '0'; //�����ּ��ַ�0�ͱ����Ӧ�ַ�
		num /= 10;				   //��ʱ���ַ���Ϊ����
	}
	tem[k] = '\0';
	k = k - 1;
	while (k >= 0)
	{
		strValue[j++] = tem[k--]; //��������ַ���תΪ����
	}
	strValue[j] = '\0'; //�ַ���������־
}
