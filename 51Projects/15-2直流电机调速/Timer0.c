#include <REGX52.H>
void Timer0Init(void) {		//1毫秒@12.000MHz

	// AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD |= 0x01;		//设置定时器模式
	TL0 = 0x9C;		//设置定时初值
	TH0 = 0xFF;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;
	EA = 1;
	PT0 = 0;
}
//            定时器中断函数模板
//  void Timer0_Routine(void) interrupt 1 {
// 	static unsigned int T0count;
// 	TL0 = 0x18;		//设置定时初值
// 	TH0 = 0xFC;	
// 	T0count++;
// 	if (T0count >= 1000) {
// 		T0count = 0;
// 		P2 = ~P2;
// 	}
// }