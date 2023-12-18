#include "bsp.h"
 
void Adc_Init(void)//ADC初始化
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_ADC1,ENABLE);//RCC->APB2ENR|=1<<2;RCC->APB2ENR|=1<<9;使能PORTA口和ADC1时钟 
 
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);//GPIOA->CRL&=0X0FFFFFFF PA7模拟输入
 
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);//RCC->CFGR&=~(3<<14);RCC->CFGR|=2<<14; 6分频ADC时钟为12M ,这个不能超多14mhz不然会出问题
 
    ADC_DeInit(ADC1);//RCC->APB2RSTR|=1<<9; ADC1复位
 
    ADC_InitStructure.ADC_Mode=ADC_Mode_Independent; //ADC1->CR1&=0XF0FFFF;ADC1->CR1|=0<<16; 独立工作模式
    ADC_InitStructure.ADC_ScanConvMode=DISABLE;//ADC1->CR1&=~(1<<8); 非扫描模式
    ADC_InitStructure.ADC_ContinuousConvMode=DISABLE;//ADC1->CR2=~(1<<1); 单次转换模式
    ADC_InitStructure.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//ADC1->CR2&=~(7<<17);ADC1->CR2|=7<<17;软件控制转换 ADC1->CR2|=1<<20; 使用外部触发
    ADC_InitStructure.ADC_DataAlign=ADC_DataAlign_Right;//ADC1->CR2&=~(1<<11);右对齐
    ADC_InitStructure.ADC_NbrOfChannel=1;//ADC1->SQR1&=~(0XF<<20); ADC1->SQR1|=0<<20;//顺序进行规则转换的ADC通道的数目1
    ADC_Init(ADC1,&ADC_InitStructure);//根据ADC_InitStructure中指定的参数初始化外设ADC的寄存器
 
    ADC_Cmd(ADC1,ENABLE);//ADC1->CR2|=0;使能ADC1
    ADC_ResetCalibration(ADC1); //ADC1->CR2|=1<<3;使能复位校准
    while(ADC_GetResetCalibrationStatus(ADC1));//while(ADC1->CR2&1<<3)等待复位校准结束
    ADC_StartCalibration(ADC1);//ADC1->CR2|=1<<2;开启AD校准
    while(ADC_GetCalibrationStatus(ADC1));//while(ADC1->CR2&1<<2)等待校准结束
}
 
//获得ADC值
//ch:通道值 0~3
u16 Get_Adc(u8 ch)   
{
    //设置指定ADC的规则组通道，一个序列，采样时间
    //ADC1->SMPR2&=~(7<<21);ADC1->SMPR2|=7<<21;ADC1,ADC通道,采样时间为239.5周期
    //ADC1->SQR3&=0XFFFFFFE0;ADC1->SQR3|=ch;
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );  
 
    //ADC1->CR2|=1<<22;                 
  //使能指定的ADC1的软件转换启动功能
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);     //使能指定的ADC1的软件转换启动功能    
 
    //while(!(ADC1->SR&1<<1)); 
    //等待转换结束
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));
 
    //return ADC1->DR
    //返回最近一次ADC1规则组的转换结果
    return ADC_GetConversionValue(ADC1);
}
 
u16 Get_Adc_Average(u8 ch,u8 times)
{
    u32 temp_val=0;
    u8 t;
    for(t=0;t<times;t++)
    {
        temp_val+=Get_Adc(ch);
        delay_us(200);
    }
    return temp_val/times;
} 
