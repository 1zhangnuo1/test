#include "bsp.h"
 
void Adc_Init(void)//ADC��ʼ��
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_ADC1,ENABLE);//RCC->APB2ENR|=1<<2;RCC->APB2ENR|=1<<9;ʹ��PORTA�ں�ADC1ʱ�� 
 
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);//GPIOA->CRL&=0X0FFFFFFF PA7ģ������
 
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);//RCC->CFGR&=~(3<<14);RCC->CFGR|=2<<14; 6��ƵADCʱ��Ϊ12M ,������ܳ���14mhz��Ȼ�������
 
    ADC_DeInit(ADC1);//RCC->APB2RSTR|=1<<9; ADC1��λ
 
    ADC_InitStructure.ADC_Mode=ADC_Mode_Independent; //ADC1->CR1&=0XF0FFFF;ADC1->CR1|=0<<16; ��������ģʽ
    ADC_InitStructure.ADC_ScanConvMode=DISABLE;//ADC1->CR1&=~(1<<8); ��ɨ��ģʽ
    ADC_InitStructure.ADC_ContinuousConvMode=DISABLE;//ADC1->CR2=~(1<<1); ����ת��ģʽ
    ADC_InitStructure.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//ADC1->CR2&=~(7<<17);ADC1->CR2|=7<<17;�������ת�� ADC1->CR2|=1<<20; ʹ���ⲿ����
    ADC_InitStructure.ADC_DataAlign=ADC_DataAlign_Right;//ADC1->CR2&=~(1<<11);�Ҷ���
    ADC_InitStructure.ADC_NbrOfChannel=1;//ADC1->SQR1&=~(0XF<<20); ADC1->SQR1|=0<<20;//˳����й���ת����ADCͨ������Ŀ1
    ADC_Init(ADC1,&ADC_InitStructure);//����ADC_InitStructure��ָ���Ĳ�����ʼ������ADC�ļĴ���
 
    ADC_Cmd(ADC1,ENABLE);//ADC1->CR2|=0;ʹ��ADC1
    ADC_ResetCalibration(ADC1); //ADC1->CR2|=1<<3;ʹ�ܸ�λУ׼
    while(ADC_GetResetCalibrationStatus(ADC1));//while(ADC1->CR2&1<<3)�ȴ���λУ׼����
    ADC_StartCalibration(ADC1);//ADC1->CR2|=1<<2;����ADУ׼
    while(ADC_GetCalibrationStatus(ADC1));//while(ADC1->CR2&1<<2)�ȴ�У׼����
}
 
//���ADCֵ
//ch:ͨ��ֵ 0~3
u16 Get_Adc(u8 ch)   
{
    //����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
    //ADC1->SMPR2&=~(7<<21);ADC1->SMPR2|=7<<21;ADC1,ADCͨ��,����ʱ��Ϊ239.5����
    //ADC1->SQR3&=0XFFFFFFE0;ADC1->SQR3|=ch;
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );  
 
    //ADC1->CR2|=1<<22;                 
  //ʹ��ָ����ADC1�����ת����������
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);     //ʹ��ָ����ADC1�����ת����������    
 
    //while(!(ADC1->SR&1<<1)); 
    //�ȴ�ת������
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));
 
    //return ADC1->DR
    //�������һ��ADC1�������ת�����
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
