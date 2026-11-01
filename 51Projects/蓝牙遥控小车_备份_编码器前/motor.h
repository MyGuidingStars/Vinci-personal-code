#ifndef __MOTOR_H__
#define __MOTOR_H__

/* 电机：L298N 双路 + 软件 PWM，速度 -100 ~ +100 */
void Motor_Init(void);
void Motor_SetTarget(signed char left, signed char right);  /* 设目标速度 */
void Motor_Stop(void);                                      /* 立即停车 */
void Motor_SetRaw(unsigned char on);                        /* 1=关闭死区补偿(调试) */
void Motor_Tick(void);                                      /* 每 20ms 调用一次 */

extern volatile unsigned char flag_20ms;                    /* 20ms 节拍标志 */

#endif
