#include <REGX52.H>

void delay (unsigned int t) {
	while (t--);
}

void main() {
	int time, i;
	while(1) {
		for (time = 0;time < 100;time++) {
			for ( i = 0;i < 20;i++) {
				P2_0 = 0;
				delay(time);
				P2_0 = 1;
				delay(100-time);
			}
		}
		for (time = 100;time > 0;time--) {
			for ( i = 0;i < 20;i++) {
				P2_0= 0;
				delay(time);
				P2_0= 1;
				delay(100-time);
			}
		}
	}
}