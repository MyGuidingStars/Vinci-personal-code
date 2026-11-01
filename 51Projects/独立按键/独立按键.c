#include <REGX52.H>
int main () {
	while (1) {
		if (P3_1 == 1) {
			P2 = 0xFF;
		}
		else {
			P2 = 0x00;
		}
	}
}