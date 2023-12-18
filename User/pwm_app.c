#include "bsp.h"

/*
*********************************************************************************************************
*	函 数 名: Pwm5_Init
*	功能说明: 设定好定时器的频率以及占空比，范围是0-1000
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Pwm5_Init(void)
{
    bsp_SetTIMOutPWM(GPIOA,GPIO_Pin_0,TIM2,1,719,1999);//设置pwm1为1000hz    设置范围是0-1000

}
void Pwm1_Init(void)
{
    bsp_SetTIMOutPWM(GPIOA,GPIO_Pin_8,TIM1,1,719,1999);//设置pwm1为1000hz    设置范围是0-1000

}
void Pwm7_Init(void)
{
    bsp_SetTIMOutPWM(GPIOA,GPIO_Pin_2,TIM2,3,719,1999);//设置pwm1为1000hz    设置范围是0-1000

}
void Pwm8_Init(void)
{
    bsp_SetTIMOutPWM(GPIOA,GPIO_Pin_3,TIM2,4,719,1999);//设置pwm1为1000hz    设置范围是0-1000

}

void Pwm4_Init(void)
{
    bsp_SetTIMOutPWM(GPIOA,GPIO_Pin_11,TIM1,4,719,1999);//设置pwm1为1000hz    设置范围是0-1000

}

void timer2_pwm_init(void)	{

	Pwm5_Init();
	Pwm1_Init();
	Pwm7_Init();
	Pwm8_Init();
	Pwm4_Init();
}





