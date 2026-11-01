#include <reg52.h>
#include "uart.h"

#define RXSIZE 32

static unsigned char idata rxbuf[RXSIZE];   /* 环形缓冲，放内部 RAM 高 128B */
static volatile unsigned char rx_head = 0;
static volatile unsigned char rx_tail = 0;
static volatile unsigned char tx_done = 1;  /* 发送完成标志，由中断置位 */

/* 9600 @ 11.0592MHz，定时器1 方式2 */
void UART_Init(void)
{
    SCON = 0x50;                 /* 方式1、8位、REN=1 允许接收 */
    TMOD &= 0x0F;                /* 只改高4位，保留定时器0 的设置 */
    TMOD |= 0x20;                /* 定时器1 方式2，自动重装 */
    TH1 = 0xFD;
    TL1 = 0xFD;
    TR1 = 1;
    ES  = 1;                     /* 串口中断允许 */
    EA  = 1;                     /* 总中断允许 */
}

/* 中断号 4：接收和发送共用，必须两个都判断 */
void UART_ISR(void) interrupt 4
{
    unsigned char c, next;

    if (RI) {
        RI = 0;                  /* 硬件置位，软件清零 */
        c = SBUF;                /* 立刻读走，腾空 1 字节缓冲 */
        next = rx_head + 1;
        if (next >= RXSIZE) next = 0;
        if (next != rx_tail) {   /* 没满才存 */
            rxbuf[rx_head] = c;
            rx_head = next;
        }
    }
    if (TI) {                    /* 发送完成 */
        TI = 0;
        tx_done = 1;
    }
}

unsigned char UART_GetChar(unsigned char *c)
{
    if (rx_head == rx_tail) return 0;
    *c = rxbuf[rx_tail];
    if (++rx_tail >= RXSIZE) rx_tail = 0;
    return 1;
}

void UART_SendByte(unsigned char b)
{
    tx_done = 0;
    SBUF = b;
    while (!tx_done);            /* 等中断里置位，别自己清 TI */
}

void UART_SendString(const char *s)
{
    while (*s) UART_SendByte((unsigned char)(*s++));
}
