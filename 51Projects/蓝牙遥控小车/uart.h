#ifndef __UART_H__
#define __UART_H__

/* 串口：9600 8N1，接收用中断 + 环形缓冲，发送用查询 */
void UART_Init(void);
unsigned char UART_GetChar(unsigned char *c);   /* 取到返回 1，无数据返回 0 */
void UART_SendByte(unsigned char b);
void UART_SendString(const char *s);

#endif
