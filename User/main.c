/*
*********************************************************************************************************
*
*	模块名称 : 主程序模块。
*	文件名称 : main.c
*	版    本 : V1.0
*	说    明 : 
*              实验目的：
*  
*              实验内容：
*   
*              注意事项：
*      
*
*	修改记录 :
*		版本号    日期         作者            说明
*       V1.0    2023-08-10          
*

*
*********************************************************************************************************
*/


#include "include.h"


/*
**********************************************************************************************************
											变量声明
**********************************************************************************************************
*/

#define M_PI    		3.1415926	
#define RAD2ANG 		(M_PI/180.0)
#define ANG2RAD(N) 		((N) * (180.0/M_PI))							


typedef struct {
	u8 data[6]; 
	u8 check_angle;						//检验三角形的
}coord_sys_t; 

typedef struct {
	unsigned char current_loca;			
}servol_t;

typedef struct {	//运动处理，
	int Current_x;		
	int Current_y;
	int Current_z;
	int Current_state; 
	u8 num;				
}Motion_Solution;

servol_t servol[6];		
Motion_Solution motion;

/*
**********************************************************************************************************
											函数声明
**********************************************************************************************************
*/
void Servol_Init(void);
void Set_speed_Allservol(u8* tar,u8 speed);					
u8 Receiving_Process(void);								
coord_sys_t Coordinate_Transformation(int x,int y,int z);	
coord_sys_t Solved_Coordinates(int x,int y,int z,int status);			
void Ease_The_Helm(void);
void Motion_execution(int target_x, int target_y, int target_z,int speed,int state);
void Ease_The_Helm_c(int x, int y);
void Clamp_or_place(u8 state);

/*
*********************************************************************************************************
*	函 数 名: main
*	功能说明: 标准c程序入口。
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
int main(void) {

	bsp_init();			//外设初始化
	Servol_Init();		//舵机初始化
    while(1) 
	{

		Receiving_Process();		//串口中断获取数据并处理
	}

}


/*
*********************************************************************************************************
*	函数名称: Coordinate_Transformation
*	函数功能: 坐标转换，将坐标转换成可读取的数据，三轴自由度
*	输入    x，y，z三轴坐标系
*	输出   舵机转动的角度
*********************************************************************************************************
*/   
coord_sys_t Coordinate_Transformation(int x,int y,int z) {
    coord_sys_t buf = {0};
	u8 Forearm  = 17;					//小臂长度
	u8 Big_arm	= 10;					//大臂长度
	float hypotenuse = 0;				//斜边
	float zzz = 0;         				//空中的斜线长度
	float zzz_angle = 0;				//需要加上的夹角
	float angle = 0;       				//底座夹角
	float angle1 = 0;	   				//大臂夹角
	float angle2 = 0;	   				//小臂夹角
	u8 angle2_flag = 0;					//求角度2的考虑
	u8 z_flag = 0;						//关于z的
	
	/* 夹取零点 */
	if(z >= 8)  {
		z -= 8;
		z_flag = 1;
	}
	else if(z < 8)  {
		z = 8-z;
		z_flag = 0;
	}

	//求解之前，必须要先确定三角形是否可求	
	//1.求出斜边
	if((x == 0||y == 0)||(x == 128)) {
		if((x == 0)||(x ==128)) hypotenuse =  y;
		else if(y == 0) {
			if(x < 128) hypotenuse = x;
			else if(x > 128) hypotenuse = x - 128;  
		}
	}
	else 
		hypotenuse = sqrt(pow(x,2) + pow(y,2));
						
	printf("hypotenuse is %f\r\n",hypotenuse);	
	zzz =  sqrt(pow(hypotenuse,2) + pow(z,2) );     //注意角度1，需要加上这个	
	zzz_angle = atan((float)z / hypotenuse) * 180.0 / M_PI;  //弧度转为角度
	if (zzz + Forearm > Big_arm && zzz + Big_arm > Forearm && Forearm + Big_arm > zzz) 
	{
        buf.check_angle = 1;
		printf("check success\r\n");
    } else {
		buf.check_angle = 0;
        printf("check error\n\n");
    }
	if(buf.check_angle > 0) {
		if(x < 128) {   					//右半轴。0-128
		
			//2.求出角度
			if(x == 0)
				angle =  0;
			else if(y == 0)
				angle = 90;
			else
				angle = atan((float)x / (float)y) * 180.0 / M_PI;	

			//3.转换第一个坐标 (注意，一个频率差对应的是1.285度)
			buf.data[0] = 176 -  (angle * 0.78);     
		
			angle1 = acos((Big_arm * Big_arm + zzz * zzz - Forearm * Forearm) / (2 * Big_arm * zzz)) * 180.0 / M_PI;  //大臂夹角
			angle2 = acos((Big_arm * Big_arm + Forearm * Forearm - zzz * zzz) / (2 * Big_arm * Forearm)) * 180.0 / M_PI;	 //小臂夹角
			
			if(z_flag == 1)
				angle1 += zzz_angle;	
			else if(z_flag == 0)
				angle1 -= zzz_angle;
			
			if((angle1 > 90)||(angle1 < 0)) {
				buf.check_angle = 0;
				printf("angle1 error\r\n");
				goto err;
			}   
					
			buf.data[1] = 55 + (( 90 - angle1) * 1.055);
			
			if(angle2_flag == 1)   
				buf.data[2] = 80 + ((angle2 - 90) * 0.759);
			else {
				if(angle2 >= 90) 
					buf.data[2] = 80 + ((angle2 - 90) * 0.759);
				else if(angle2 < 90) 
					buf.data[2] = 80 - ((90 - angle2) * 0.759);
			}
		}
		else if(x >= 128) {   					//左半轴
		    x -= 128;
			//2.求出角度
			if(x == 0)
				angle =  0;
			else if(y == 0)
				angle = 90;
			else
				angle = atan((float)x / (float)y) * 180.0 / M_PI;	

			//3.转换第一个坐标 (注意，一个频率差对应的是1.285度)
			buf.data[0] = 176 +  (angle * 0.78);      

			angle1 = acos((Big_arm * Big_arm + zzz * zzz - Forearm * Forearm) / (2 * Big_arm * zzz)) * 180.0 / M_PI;  //大臂夹角
			angle2 = acos((Big_arm * Big_arm + Forearm * Forearm - zzz * zzz) / (2 * Big_arm * Forearm)) * 180.0 / M_PI;	 //小臂夹角
			if(z_flag == 1)
				angle1 += zzz_angle;	
			else if(z_flag == 0)
				angle1 -= zzz_angle;

			if(angle1 > 90) {
				buf.check_angle = 0;
				printf("angle1 error > 90\r\n");
				goto err;
			}    		
			buf.data[1] = 55 + (( 90 - angle1) * 1.055);
			
			if(angle2_flag == 1)   
				buf.data[2] = 80 + ((angle2 - 90) * 0.759);
			else {
				if(angle2 >= 90) 
					buf.data[2] = 80 + ((angle2 - 90) * 0.759);
				else if(angle2 < 90) 
					buf.data[2] = 80 - ((90 - angle2) * 0.759);
			}
		}
	}
	
err:	
	return buf;
}

/*
*********************************************************************************************************
*	函数名称: Solved_Coordinates
*	函数功能: 坐标转换，将坐标转换成可读取的数据，四轴自由度
*	输入    x，y，z三轴坐标系
*	输出   舵机转动的角度
*********************************************************************************************************
*/ 
coord_sys_t Solved_Coordinates(int x,int y,int z,int status) {
	coord_sys_t buf = {0};
	double a, b; 							//临时变量
	double L1 = 10, L2 = 10, L3 = 12;		//3节手臂的长度
	double m, n, t, q, p;					//临时变量
	double j1, j2, j3, j0;					//4个舵机的旋转角度
	double last_j[4];						//主要用于找到最适合的角度	
	double x1, y1, z1;						//逆解后正解算出来的值，看是否与逆解值相等
	int angle[4] ={0};
	char i = 0;   
	u8 x_flag = 0;							//判断是正半轴还是负半轴,0为正半轴，y为负半轴
	if(x >= 128) {
		x =  x - 128 ;
		x_flag = 1;
	}
	else
		x_flag = 0;
	z -= 7;
	j0 = atan2(y, x);
	a = x / cos(j0);
	if (x == 0) a = y; //如果x为0，需要交换x，y
	b = z;

	for (j1 = 0; j1 < 90; j1++)
	{	
		j1 *= RAD2ANG;
		j3 = acos((pow(a, 2) + pow(b, 2) + pow(L1, 2) - pow(L2, 2) - pow(L3, 2) - 2 * a * L1 * sin(j1) - 2 * b * L1 * cos(j1)) / (2 * L2 * L3));
		m = L2 * sin(j1) + L3 * sin(j1) * cos(j3) + L3 * cos(j1) * sin(j3);
		n = L2 * cos(j1) + L3 * cos(j1) * cos(j3) - L3 * sin(j1) * sin(j3);
		t = a - L1 * sin(j1);
		p = pow(pow(n, 2) + pow(m, 2), 0.5);
		q = asin(m / p);
		j2 = asin(t / p) - q;
		/***************计算正解然后与目标解对比，看解是否正确**************/
		x1 = (L1 * sin(j1) + L2 * sin(j1 + j2) + L3 * sin(j1 + j2 + j3)) * cos(j0);
		y1 = (L1 * sin(j1) + L2 * sin(j1 + j2) + L3 * sin(j1 + j2 + j3)) * sin(j0);
		z1 = L1 * cos(j1) + L2 * cos(j1 + j2) + L3 * cos(j1 + j2 + j3);
		j1 = ANG2RAD(j1);
		j2 = ANG2RAD(j2);
		j3 = ANG2RAD(j3);
		if (x1<(x + 1) && x1 >(x - 1) && y1<(y + 1) && y1 >(y - 1) && z1<(z + 1) && z1 >(z - 1))
		{
			if(status == 0x01) {
			if (j3 > 0) {   //寻找到最大的角度，且同时必须符合角度要求

					if((j2 >= last_j[2]))
					{
						last_j[0] = j0;
						last_j[1] = j1;
						last_j[2] = j2;
						last_j[3] = j3;
						i = 1;
					}	
				}
				else
				 goto check_1_success;
			}
		}
	}

check_1_success:

	for (j1 = 0; j1 < 90; j1++)//这个循环是为了求解另一组解，j2 = asin(t / p) - q;j2 = -(asin(t / p) - q);多了个负号
	{
		j1 *= RAD2ANG;
		j3 = acos((pow(a, 2) + pow(b, 2) + pow(L1, 2) - pow(L2, 2) - pow(L3, 2) - 2 * a * L1 * sin(j1) - 2 * b * L1 * cos(j1)) / (2 * L2 * L3));
		m = L2 * sin(j1) + L3 * sin(j1) * cos(j3) + L3 * cos(j1) * sin(j3);
		n = L2 * cos(j1) + L3 * cos(j1) * cos(j3) - L3 * sin(j1) * sin(j3);
		t = a - L1 * sin(j1);
		p = pow(pow(n, 2) + pow(m, 2), 0.5);
		q = asin(m / p);
		j2 = -(asin(t / p) - q);
		x1 = (L1 * sin(j1) + L2 * sin(j1 + j2) + L3 * sin(j1 + j2 + j3)) * cos(j0);
		y1 = (L1 * sin(j1) + L2 * sin(j1 + j2) + L3 * sin(j1 + j2 + j3)) * sin(j0);
		z1 = L1 * cos(j1) + L2 * cos(j1 + j2) + L3 * cos(j1 + j2 + j3);
		j1 = ANG2RAD(j1);
		j2 = ANG2RAD(j2);
		j3 = ANG2RAD(j3);
		if (x1<(x + 1) && x1 >(x - 1) && y1<(y + 1) && y1 >(y - 1) && z1<(z + 1) && z1 >(z - 1))
		{		
			if(status == 0x01) {   //如果爪子是处于开启状态
				if (j3 > 0) {   //寻找到最大的角度，且同时必须符合角度要求
					if((j2 >= last_j[2]))
					{
						last_j[0] = j0;
						last_j[1] = j1;
						last_j[2] = j2;
						last_j[3] = j3;
						i = 1;
					}	
				}
				else 
				 goto check_2_success;
			}
		}
	}
check_2_success:
	j0 = last_j[0];
	j0 = ANG2RAD(j0);
	j1 = last_j[1];
	j2 = last_j[2];
	j3 = last_j[3];

	if(x_flag == 1)
		j0 = 180 - j0;

	printf("j0:%f,j1:%f,j2:%f,j3:%f\r\n", j0, j1, j2, j3);
	if(i == 0) {
		buf.check_angle = 0; 
		printf("error\r\n");
	}
	else buf.check_angle = 1;

	if(j0 <= 90) {
		angle[0] = 90 - j0;	
		buf.data[0] = 176 -  (angle[0] * 0.78);
	}
	else if(j0 > 90) {
		angle[0] = j0 - 90;
		buf.data[0] = 176 +  (angle[0] * 0.78);
	}
	if(j1 >= 0) { 
		angle[1] = j1;
		buf.data[1] = 55 + angle[1] * 1.055;
	}
	else {
		printf("angle error\r\n");
		buf.check_angle = 0; 
	}
	angle[2] = j2;
	buf.data[2] = 150 - angle[2] * 0.759;
	 
	angle[3] = j3;
	buf.data[3] = 150 - angle[3] * 0.759;

    return buf;
}


/*
 *  Motion_execution
 *  处理从当前数值到目标值的解算，算了，不求太难的了，就实现简单的升高转动，再放置，在归位吧
 *  输入 目标值，state: 0运行后再进行夹取，2运行后再进行放置
 *  输出 无
 */
void Motion_execution(int target_x, int target_y, int target_z,int speed,int state)
{
	coord_sys_t buf;	
	
	if(state == 0)	//表示夹取
	{
		Ease_The_Helm_c(target_x,target_y);
		buf = Solved_Coordinates(target_x,target_y,target_z,1);
		if(buf.check_angle == 1)
			Set_speed_Allservol(buf.data,speed);
		Clamp_or_place(state);
		Ease_The_Helm_c(target_x,target_y);
	}
	else if(state == 2)	//放置
	{
		Ease_The_Helm_c(target_x,target_y);
		buf = Solved_Coordinates(target_x,target_y,target_z,1);	
		if(buf.check_angle == 1)
			Set_speed_Allservol(buf.data,speed);
		Clamp_or_place(state);
		Ease_The_Helm_c(target_x,target_y);
	}
}

/*
 *  Describing_Circle
 *  控制机械臂画圆
 *  输入 z轴的高度，y轴的距离,半径,运行速度，状态
 *  输出 x，y,z的参数
 */
void Describing_Circle(int x, int y, int z, int radius, int speed, int state)
{
    float angle = 0;
	coord_sys_t serv;
    int target_x = 0, target_y = 0, target_z = 0;

    for (angle = 0; angle < 360; angle+=5)
    {
        target_x = x + (int)(radius * cos(angle * M_PI / 180.0));
        target_z = z + (int)(radius * sin(angle * M_PI / 180.0));

        target_y = y;
		serv = Solved_Coordinates(target_x,target_y,target_z,state);
		if(serv.check_angle == 1)
			Set_speed_Allservol(serv.data,speed);

		printf("target_x = %d, target_y = %d, target_z = %d\r\n", target_x, target_y, target_z);
			
    }
}


/*
 *  Receiving_Process
 *  函数主要用于接收数据控制舵机
 *  输入 无
 *  输出 0正确 -1 错误
 */
u8 Receiving_Process(void) {
    int res = 0;
	coord_sys_t ser;		
	int x,y,z;				
	u8 speed;
	int state;
	u8 len;
	if(USART_RX_STA&0x8000)  {     
		
		len = USART_RX_STA & 0x3fff;
		printf("len is %d\r\n",len);
		switch(len) {
			//1,4,5,6,7为调试用的，比较重要的将采用状态机模式在主循环里面一直执行
			//这样可以避免只能执行舵机控制一个任务
			case 1 :        
				if(USART_RX_BUF[0] == 0x13)
				{
					Ease_The_Helm();
				}
			break;
			
			case 4 :         
				if(USART_RX_BUF[0] == 0x12)
				{
					  switch(USART_RX_BUF[1])  {
						  case 0x01:  TIM_SetCompare1(TIM2,USART_RX_BUF[2]);
						  break;
						  case 0x02:  TIM_SetCompare4(TIM1,USART_RX_BUF[2]);
						  break;
						  case 0x03:  TIM_SetCompare3(TIM2,USART_RX_BUF[2]);
						  break;
						  case 0x04:  TIM_SetCompare4(TIM2,USART_RX_BUF[2]);
						  break;
					  }
				}
			break;
			
			case 5 :        
				if(USART_RX_BUF[0] == 0x12)
				{
					x = USART_RX_BUF[1];
					y = USART_RX_BUF[2];
					z = USART_RX_BUF[3];
					speed = USART_RX_BUF[4];
					ser = Coordinate_Transformation(x,y,z);
					if(ser.check_angle == 1)
						Set_speed_Allservol(ser.data,speed);
				}
			break;
				
			case 6 :        
				if(USART_RX_BUF[0] == 0x12)
				{
					x = USART_RX_BUF[1];
					y = USART_RX_BUF[2];
					z = USART_RX_BUF[3];
					speed = USART_RX_BUF[4];
					state = USART_RX_BUF[5];
					if(state == 0x01)//直接到达
					{
						ser = Solved_Coordinates(x,y,z,state);
						if(ser.check_angle == 1)
							Set_speed_Allservol(ser.data,speed);
					}
					else
						Motion_execution(x, y,z,speed,state);						
				}
			break;

			case 7 :        
				if(USART_RX_BUF[0] == 0x12)
				{
					x = USART_RX_BUF[1]; 
					y = USART_RX_BUF[2];
					z = USART_RX_BUF[3];
					speed = USART_RX_BUF[5];
					Describing_Circle(x,y,z,USART_RX_BUF[4],speed,USART_RX_BUF[6]);
				}
			break;
		
			default :
			    res = -1;
				printf("Serial transmission error\r\n");
			
			break;
				
		}	
		USART_RX_STA = 0;
	}
	
	return res;
	
}  


/*
 *  Clamp_or_place
 *  夹取函数
 *  输入 夹取哪个东西
 *  输出 无
 */
void Clamp_or_place(u8 state)
{
	if(state == 0)
	{
		TIM_SetCompare1(TIM1,50);              //50-250
		delay_ms(500);
		TIM_SetCompare1(TIM1,150);
		delay_ms(1000);
	}
	else
	{
		TIM_SetCompare1(TIM1,250);              //50-250
		delay_ms(500);
		TIM_SetCompare1(TIM1,150);
		delay_ms(1000);
	}
		
}
   

/*
 *  Set_speed_servol
 *  舵机整体控制函数
 *  输入
 * 		 id确定舵机的id
 * 		 目标值 50-250
 *		 速度  0x01,0x02,0x03
 *  输出 无
 */
void Set_speed_Allservol(u8* tar,u8 speed) {

	int i = 0;			     
	u8 current[4] = {0};		   
	int interval[4] = {0};	
	int Minimum_difference;
	
	/******************
	 * 将所有的数据进行限制，因为大于也没有什么用
	 * 
	******************/
	if(tar[0] > 255) 		tar[0] = 255;
	else if((tar[0] < 50)&&(tar[0] > 0))	tar[0] = 50;

	if(tar[1] > 150) 		tar[1] = 150;
	else if((tar[1] < 55)&&(tar[1] > 0)) 	tar[1] = 55;

	if(tar[2] > 240) 		tar[2] = 240;
	else if((tar[2] < 50)&&(tar[2] > 0)) 	tar[2] = 50;

	if(tar[3] > 240) 		tar[3] = 240;
	else if((tar[3] < 50)&&(tar[3] > 0)) 	tar[3] = 50;

	if(tar[0] == 0) tar[0] = 175;
	if(tar[1] == 0) tar[1] = 55;
	if(tar[2] == 0) tar[2] = 150;
	if(tar[3] == 0) tar[3] = 150;


	/******************
	 * 计算差值，差值最终会以interval表达出来
	 * 
	******************/
	for(i = 0; i < 4; i++) 
	{
		current[i] = servol[i].current_loca;	
		interval[i] = tar[i] - current[i];
		if(Minimum_difference < abs(interval[i]))
			Minimum_difference = abs(interval[i]);
	}
	
	for(i=0;i<Minimum_difference;i++) {

		if((interval[0] >= 0)&&(interval[0] < 206)) {
			if(i < interval[0]) current[0]++;    
		}
		else if((interval[0] < 0)&&(interval[0] > -206))  {
			if(i < -interval[0]) current[0]--;
        }
		
		if((interval[1] >= 0)&&(interval[1] < 101))  {
		   if(i < interval[1]) current[1]++;
		}   	
		else if((interval[1] < 0)&&(interval[1] > -101)) {
			if(i < -interval[1]) current[1]--;
		}
		
		if((interval[2] >= 0)&&(interval[2] < 191))    {
			if(i < interval[2]) current[2]++;
		}
		else if((interval[2] < 0)&&(interval[2] > -191)) {
			if(i < -interval[2]) current[2]--;
		}
		
		if((interval[3] >= 0)&&(interval[3] < 181)) {
			if(i < interval[3]) current[3]++;
		}
		else if((interval[3] < 0)&&(interval[3] > -180))  {
			if(i < -interval[3]) current[3]--;
		}

		TIM_SetCompare1(TIM2,current[0]);
		TIM_SetCompare4(TIM1,current[1]);
		TIM_SetCompare3(TIM2,current[2]);
		TIM_SetCompare4(TIM2,current[3]);
		if(speed == 0x01) 	
			delay_ms(10);
		else if(speed == 0x02) 
			delay_ms(20);
		else if(speed == 0x03) 
			delay_ms(50);
	}

	//写入当前的位置
	for(i = 0; i < 4; i++) 
		servol[i].current_loca = current[i]; 		

}

/*
 *  Ease_The_Helm
 *  舵机回中函数
 *  输入 无
 *  输出 无
 */

void Ease_The_Helm(void) {

	int i = 0;			     		 //用于for循环
	u8 current[4] = {0};		     //当前的数值
	int interval[4] = {0};	     	 //差值，有负数
    int Minimum_difference;	
	current[0] = servol[0].current_loca;	
	interval[0] = 175 - current[0];

	current[1] = servol[1].current_loca;	
	interval[1] = 55 - current[1];

	current[2] = servol[2].current_loca;	
	interval[2] = 150 - current[2];

	current[3] = servol[3].current_loca;	
	interval[3] = 150 - current[3];
	
	for(i =0;i<4;i++)
	{
		if(Minimum_difference < abs(interval[i]))
			Minimum_difference = abs(interval[i]);
	}
		

	for(i=0;i<Minimum_difference;i++) {

		if((interval[0] >= 0)&&(interval[0] < 206)) {
			if(i < interval[0]) current[0]++;    
		}
		else if((interval[0] < 0)&&(interval[0] > -206))  {
			if(i < -interval[0]) current[0]--;
        }
		
		if((interval[1] >= 0)&&(interval[1] < 101))  {
		   if(i < interval[1]) current[1]++;
		}   	
		else if((interval[1] < 0)&&(interval[1] > -101)) {
			if(i < -interval[1]) current[1]--;
		}
		
		if((interval[2] >= 0)&&(interval[2] < 191))    {
			if(i < interval[2]) current[2]++;
		}
		else if((interval[2] < 0)&&(interval[2] > -191)) {
			if(i < -interval[2]) current[2]--;
		}
		
		if((interval[3] >= 0)&&(interval[3] < 181)) {
			if(i < interval[3]) current[3]++;
		}
		else if((interval[3] < 0)&&(interval[3] > -180))  {
			if(i < -interval[3]) current[3]--;
		}

		TIM_SetCompare1(TIM2,current[0]);
		TIM_SetCompare4(TIM1,current[1]);
		TIM_SetCompare3(TIM2,current[2]);
		TIM_SetCompare4(TIM2,current[3]);
		delay_ms(20);

	}

	//写入当前的位置
	for(i = 0; i < 4; i++) 
		servol[i].current_loca = current[i]; 		
}

/*
 *  Ease_The_Helm_c
 *  舵机回中函数(此函数用于放置)，输入x，y的坐标，将底座角度算好
 *  输入 x,y坐标
 *  输出 无
 */

void Ease_The_Helm_c(int x, int y) {

	int i = 0;			     		
	u8 current[4] = {0};		    
	int interval[4] = {0};	     	
	double angle;					
	int data;						
    int Minimum_difference;
	if(x > 128)
	{
		x-=128;
		angle = atan((float)x / (float)y) * 180.0 / M_PI;	
		data = 176 +  (angle * 0.78);  
	}
	else 
	{
		angle = atan((float)x / (float)y) * 180.0 / M_PI;	
		data = 176 -  (angle * 0.78);  
	}
	current[0] = servol[0].current_loca;	
	interval[0] = data - current[0];

	current[1] = servol[1].current_loca;	
	interval[1] = 55 - current[1];

	current[2] = servol[2].current_loca;	
	interval[2] = 150 - current[2];

	current[3] = servol[3].current_loca;	
	interval[3] = 150 - current[3];
	
	for(i =0;i<4;i++)
	{
		if(Minimum_difference < abs(interval[i]))
			Minimum_difference = abs(interval[i]);
	}	

	for(i=0;i<Minimum_difference;i++) {

		if((interval[0] >= 0)&&(interval[0] < 206)) {
			if(i < interval[0]) current[0]++;    
		}
		else if((interval[0] < 0)&&(interval[0] > -206))  {
			if(i < -interval[0]) current[0]--;
        }
		
		if((interval[1] >= 0)&&(interval[1] < 101))  {
		   if(i < interval[1]) current[1]++;
		}   	
		else if((interval[1] < 0)&&(interval[1] > -101)) {
			if(i < -interval[1]) current[1]--;
		}
		
		if((interval[2] >= 0)&&(interval[2] < 191))    {
			if(i < interval[2]) current[2]++;
		}
		else if((interval[2] < 0)&&(interval[2] > -191)) {
			if(i < -interval[2]) current[2]--;
		}
		
		if((interval[3] >= 0)&&(interval[3] < 181)) {
			if(i < interval[3]) current[3]++;
		}
		else if((interval[3] < 0)&&(interval[3] > -180))  {
			if(i < -interval[3]) current[3]--;
		}

		TIM_SetCompare1(TIM2,current[0]);
		TIM_SetCompare4(TIM1,current[1]);
		TIM_SetCompare3(TIM2,current[2]);
		TIM_SetCompare4(TIM2,current[3]);
		delay_ms(20);

	}

	//写入当前的位置
	for(i = 0; i < 4; i++) 
		servol[i].current_loca = current[i]; 		

}

/*  舵机初始化 Servol_Init
 *  将舵机结构体都初始化
 *  输入
 *  输出
 */
void Servol_Init(void)
{
	//初始化当前位置
	servol[0].current_loca = 175;
	servol[1].current_loca = 55;
	servol[2].current_loca = 150;
	servol[3].current_loca = 150;
	servol[4].current_loca = 150;
	servol[5].current_loca = 240;

	TIM_SetCompare1(TIM2,175);              //50-250   
	delay_ms(100);
	TIM_SetCompare4(TIM1,55);               //50-150  
	delay_ms(100);
	TIM_SetCompare3(TIM2,150);              //60-240 
	delay_ms(100);
	TIM_SetCompare4(TIM2,150);              //70-220   
	delay_ms(100);
	TIM_SetCompare1(TIM1,150);              //70-220

	//初始化转换的这个
	motion.num = 0;
	motion.Current_x = 0;
	motion.Current_y = 0;
	motion.Current_z = 0;	
}  
  


