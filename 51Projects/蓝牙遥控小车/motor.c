#include <reg52.h>
#include "motor.h"

/* ---------------- 引脚（按你的板子改这里） ----------------
   P1.0 ENA  P1.1 IN1  P1.2 IN2   -> 左电机
   P1.3 ENB  P1.4 IN3  P1.5 IN4   -> 右电机
   注意：普中板 P1.5 常接蜂鸣器，若冲突把 ENB 改到 P2.0，
        同时把下面的 sbit ENB = P1^5; 改成 P2^0;
---------------------------------------------------------- */
sbit ENA = P1^0;
sbit IN1 = P1^1;
sbit IN2 = P1^2;
sbit ENB = P1^3;
sbit IN3 = P1^4;
sbit IN4 = P1^5;

#define PWM_LEVELS 20        /* PWM 20 级，5% 一档 */
#define RAMP_STEP  5         /* 每 20ms 加/减 5 级 -> 0~100 约 400ms */

/* 死区补偿：占空比低于这个值电机根本转不起来。
   实测方法：发 D 进调试 → 按 1（左轮）→ 用 + 慢慢加到刚转起来，记下 ? 里的 S 值；
   右轮按 4 同样测。测出来的数填到这里，之后任意非零速度都会从死区往上给。 */
#define DEAD_L     15        /* 左轮死区 %（实测值） */
#define DEAD_R     10        /* 右轮死区 %（实测值） */

static volatile unsigned char pwm_cnt = 0;
static volatile unsigned char pwm_a = 0;    /* 左轮占空 0~20 */
static volatile unsigned char pwm_b = 0;    /* 右轮占空 0~20 */
static volatile unsigned char tick20 = 0;

volatile unsigned char flag_20ms = 0;       /* 主循环用的 20ms 节拍 */

static signed char cur_l = 0, cur_r = 0;    /* 当前速度（斜坡后的） */
static signed char tgt_l = 0, tgt_r = 0;    /* 目标速度 */
static unsigned char raw_mode = 0;          /* 1=不补偿死区，直接给占空比 */

void Motor_SetRaw(unsigned char on)
{
    raw_mode = on;
}

void Motor_Init(void)
{
    TMOD = (TMOD & 0xF0) | 0x02;   /* 定时器0 方式2，自动重装 */
    TH0 = 0x48;                    /* 256-184=72，184×1.085us ≈ 200us */
    TL0 = 0x48;
    ET0 = 1;
    TR0 = 1;

    ENA = 0; ENB = 0;
    IN1 = 0; IN2 = 0; IN3 = 0; IN4 = 0;
}

/* 200us 一次：产生 20 级 PWM，同时分频出 20ms 节拍 */
void Timer0_ISR(void) interrupt 1
{
    if (++pwm_cnt >= PWM_LEVELS) pwm_cnt = 0;

    ENA = (pwm_cnt < pwm_a);
    ENB = (pwm_cnt < pwm_b);

    if (++tick20 >= 100) {         /* 100 × 200us = 20ms */
        tick20 = 0;
        flag_20ms = 1;
    }
}

/* 速度 0~100 -> 占空比级数：从死区开始线性映射到 100% */
static unsigned char duty_of(unsigned char mag, unsigned char dead)
{
    unsigned int p;
    if (mag == 0) return 0;
    if (raw_mode) return (unsigned char)((unsigned int)mag * PWM_LEVELS / 100);
    if (dead > 80) dead = 80;
    p = (unsigned int)dead + ((unsigned int)(100 - dead) * mag) / 100;
    if (p > 100) p = 100;
    return (unsigned char)(p * PWM_LEVELS / 100);
}

/* 把当前速度写到方向脚和占空比上 */
static void apply(void)
{
    if (cur_l > 0) {
        IN1 = 1; IN2 = 0;
        pwm_a = duty_of((unsigned char)cur_l, DEAD_L);
    } else if (cur_l < 0) {
        IN1 = 0; IN2 = 1;
        pwm_a = duty_of((unsigned char)(-cur_l), DEAD_L);
    } else {
        IN1 = 0; IN2 = 0;          /* 停止：两脚同低，电机自由滑行 */
        pwm_a = 0;
    }

    if (cur_r > 0) {
        IN3 = 1; IN4 = 0;
        pwm_b = duty_of((unsigned char)cur_r, DEAD_R);
    } else if (cur_r < 0) {
        IN3 = 0; IN4 = 1;
        pwm_b = duty_of((unsigned char)(-cur_r), DEAD_R);
    } else {
        IN3 = 0; IN4 = 0;
        pwm_b = 0;
    }
}

void Motor_SetTarget(signed char left, signed char right)
{
    if (left >  100) left  =  100;
    if (left < -100) left  = -100;
    if (right >  100) right =  100;
    if (right < -100) right = -100;

    tgt_l = left;
    tgt_r = right;
}

void Motor_Stop(void)
{
    tgt_l = 0; tgt_r = 0;
    cur_l = 0; cur_r = 0;
    apply();
}

/* 每 20ms 靠近目标一步，避免突然正反转烧驱动 */
void Motor_Tick(void)
{
    if (cur_l < tgt_l) {
        cur_l += RAMP_STEP;
        if (cur_l > tgt_l) cur_l = tgt_l;
    } else if (cur_l > tgt_l) {
        cur_l -= RAMP_STEP;
        if (cur_l < tgt_l) cur_l = tgt_l;
    }

    if (cur_r < tgt_r) {
        cur_r += RAMP_STEP;
        if (cur_r > tgt_r) cur_r = tgt_r;
    } else if (cur_r > tgt_r) {
        cur_r -= RAMP_STEP;
        if (cur_r < tgt_r) cur_r = tgt_r;
    }

    apply();
}
