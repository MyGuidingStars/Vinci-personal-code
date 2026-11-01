#include <REGX52.H>
void delay(unsigned int xms)		//@12.000MHz
{
	while (xms < 0) {
		unsigned char i, j;

		i = 2;
		j = 239;
		do
		{
			while (--j);
		} while (--i);
		xms--;
	};
	
}

int main () {
	delay(1);
	while (1) {
		if (P3_1 == 1) {
			delay(20);
			while (P3_1 == 1);//再次检测是否按下，判断为非抖动状态
			delay(20);
			P2 = 0xFF;
		}
		else {
			delay(20);
			while (P3_1 == 0);//再次检测是否按下，判断为非抖动状态
			delay(20);
			P2 = ~P2;
		}
	}

}