#include <REGX52.H>
#include "Timer0.h"
unsigned int compare, counter;
void Timer0_Routine(void) interrupt 1 {
	static unsigned int T0count;
	TL0 = 0x18;		//设置定时初值
	TH0 = 0xFC;	
	T0count++;
	counter %= 100;
	if (counter >= compare) {
		P3_6 = 0;
		P3_7 = 0;
	}
	else {
		P3_6 = 1;
		P3_7 = 1;
	}
}

int main () {
	Timer0Init ();
	compare = 70;
	while(1) {
		
	}
}



