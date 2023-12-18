#include "usart3.h"

#define USART3_RXBUF_LEN 15
u8 t;
u8 USART3_RX_BUF[USART3_MAX_RECV_LEN]; 				//接收缓冲,最大USART3_MAX_RECV_LEN个字节.
u8 USART3_TX_BUF[USART3_MAX_SEND_LEN]; 				//发送缓冲,最大USART3_MAX_SEND_LEN字节

vu16 USART3_RX_STA=0;

//初始化串口3，使用9600波特率，和esp8266设备进行通信 
//bound:波特率
void uart3_init(u32 bound){
	//初始化结构体
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
	//外部时钟初始化
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE); //使能GPIOB时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//使能USART3时钟
	
	USART_DeInit(USART3);  //复位串口3
    //USART3_TX   PB.10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PB.10
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
    GPIO_Init(GPIOB, &GPIO_InitStructure); //初始化PB10

    //USART2_RX    PB.11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);  //初始化PB11

	//串口初始化
    USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode =  USART_Mode_Rx | USART_Mode_Tx;	// 收发模式  

    //Usart3 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//串口3中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
	USART_Init(USART3, &USART_InitStructure); //初始化串口
	USART_Cmd(USART3, ENABLE);  //使能串口  
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启相关中断
	
	TIM7_Int_Init(1000-1,7200-1);		//10ms中断
	USART3_RX_STA=0;		//清零
	TIM_Cmd(TIM7,DISABLE);			//关闭定时器7
}

//串口3的发送函数 len=USART_RX_STA&0x3fff;
//得到此次接收到的数据长度USART_SendData(USART1, USART_RX_BUF[t]);
void Uart3_SendStr(u8* SendBuf,u8 len)
{
	//len=SendBuf&0x3fff;//得到此次接收到的数据长度
	for(t=0;t<len;t++)
	{
		USART_SendData(USART3, SendBuf[t]);//向串口1发送数据
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
	}
	printf("\r\n\r\n");//插入换行
}

//串口3,printf 函数
//确保一次发送数据不超过USART3_MAX_SEND_LEN字节
void u3_printf(char* fmt,...)
{
	
	u16 i,j; 
	va_list ap; 

	va_start(ap,fmt);
	vsprintf((char*)USART3_TX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)USART3_TX_BUF);		//此次发送数据的长度
	for(j=0;j<i;j++)							//循环发送数据
	{
	  while(USART_GetFlagStatus(USART3,USART_FLAG_TC)==RESET); //循环发送,直到发送完毕   
		USART_SendData(USART3,USART3_TX_BUF[j]); 
	}
}

// 当串口三收到数据, 系统自动调用此中断函数
void USART3_IRQHandler(void)
{
	u8 res = 0;	
	
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)//接收到数据
	{	 
		res = USART_ReceiveData(USART3);	
		
		if((USART3_RX_STA&(1<<15))==0)//接收完的一批数据,还没有被处理,则不再接收其他数据
		{
			if(USART3_RX_STA<USART3_MAX_RECV_LEN)	//还可以接收数据
			{
				TIM_SetCounter(TIM7,0);//计数器清空          				//计数器清空
				if(USART3_RX_STA==0) 				//使能定时器7的中断 
				{
					TIM_Cmd(TIM7,ENABLE);//使能定时器7
				}
				USART3_RX_BUF[USART3_RX_STA++]=res;	//记录接收到的值
			} else 
			{
				USART3_RX_STA|=1<<15;				//强制标记接收完成
			}
		}
	}  				 											 
}

// void USART3_IRQHandler(void)
// {
// 	u8 Res;
// 	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)//接收到数据
// 	{
// 		Res =USART_ReceiveData(USART3);
// 		if((USART3_RX_STA&0x8000)==0)//接收完的一批数据,还没有被处理,则不再接收其他数据
// 		{
// 		    if((USART3_RX_STA&0X7FFF)<USART3_MAX_RECV_LEN)	//还可以接收数据
// 				{
// 					if(Res!='!')
// 					{
// 						USART3_RX_BUF[USART3_RX_STA++]=Res;	//记录接收到的值
// //						printf("%c\r\n",Res);
// 					}
//           else
// 					{
// 						USART3_RX_STA|=0x8000;	//则信息接收完成了
// 					}
// 				}
// 				else
// 				{
// 					USART3_RX_STA|=0x8000;	//则信息接收完成了
// 				}
// 		}
// 		USART3_RX_Data();
// 	}
// }
