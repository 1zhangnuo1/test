#ifndef __USART3_H
#define __USART3_H


#include <stdarg.h>
#include <stdio.h>	 	 
#include <string.h>	 
#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "timer.h"


#define USART3_MAX_RECV_LEN 1024
#define USART3_MAX_SEND_LEN 1024

extern vu16 USART3_RX_STA;         		//����״̬���
extern u8 USART3_RX_BUF[USART3_MAX_RECV_LEN]; 		//���ջ���,���USART3_MAX_RECV_LEN���ֽ�.
extern u8 USART3_TX_BUF[USART3_MAX_SEND_LEN]; 		//���ͻ���,���USART3_MAX_SEND_LEN�ֽ�

void uart3_init(u32 bound);
void u3_printf(char* fmt,...);
void Uart3_SendStr(u8* SendBuf,u8 len);
//void send_data(u8 hongwai,u8 hujiao);
#endif
