#include <REGX52.H>
#include "Timer0.h"
unsigned char counter, compare;

void Timer0_Routine(void) interrupt 1 {
	TL0 = 0x9C;		//设置定时初值
	TH0 = 0xFF;
	counter++;
	counter %= 100;    //
	if (counter >= compare) {
		P1_0 = 0;
	}
	else {
		P1_0 = 1;
	}
}

void main() {
	Timer0Init ();
	compare = 70;
	while(1) {
		
	}
}

