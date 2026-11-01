#include <REGX52.H>
#define RXSIZE 32                      // 环形缓冲区大小

unsigned char rxbuf[RXSIZE];          // 接收数据的缓冲区
unsigned char volatile rx_head = 0;    // 写指针（中断里改）
unsigned char volatile rx_tail = 0;    // 读指针（主循环里改）
unsigned int  volatile rx_drop = 0;    // 缓冲区满了丢掉的字节数
void UART_ISR(void) interrupt 4
{
    unsigned char c, next;

    if (RI) {                     // 是“收”触发的中断
        RI = 0;                   // 先清零（顺序无所谓，但别忘）
        c  = SBUF;                // 读走，硬件缓冲立刻腾空
        next = rx_head + 1;       // 算下一个写位置
        if (next >= RXSIZE) next = 0;   // 回卷
        if (next != rx_tail) {    // 没满
            rxbuf[rx_head] = c;
            rx_head = next;
        } else {
            rx_drop++;            // 满了，丢弃
        }
    }
    if (TI) TI = 0;               // 是“发”触发的中断（发送完成）
}