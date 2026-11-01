#include <reg52.h>
#include "uart.h"
#include "motor.h"

/* ================= 通用参数 ================= */
#define SPEED_DEF       60     /* 手动模式默认速度 */
#define TURN_PCT        60     /* 原地转弯速度比例 */
#define TRIM_R          160    /* 右轮补偿：实测值（右轮弱 60%），换电机后要重调 */
#define LINK_TIMEOUT_MS 0      /* 断线保护，0=关 */
#define ACK_ENABLE      1      /* 回 OK */
#define DEBUG_ECHO      1      /* 回显收到的字节 */

/* ================= 循迹参数（按实际调） ================= */
#define LINE_ACTIVE_LOW 0      /* 0=压黑线输出高电平（本车实测）；1=输出低 */
#define TRACK_SPEED     30     /* 进入自动模式的基础速度（跑出去就调小） */
#define TRACK_KP        10     /* 比例系数 /10 */
#define TRACK_KD        4      /* 微分系数 /10 */
#define TRACK_KI        0      /* 积分系数 /10；0=关闭（回到纯 PD） */
#define TRACK_MAX       55     /* 转向量限幅 */
#define CORNER_ON       5      /* 同时压线 >=5 个，判定为直角弯 */
#define BEEP_MS         500    /* 声光提示持续时间(ms) */

/* 声光：低电平有效（有源蜂鸣器、LED 阳极接 VCC） */
sbit BUZZER = P1^6;
sbit LED    = P1^7;

/* ================= 状态 ================= */
static unsigned char speed = SPEED_DEF;
static unsigned char trim_r = TRIM_R;   /* 右轮补偿，可在手机上用 + / - 调 */
static char dir = 'S';
static unsigned char mode = 0;     /* 0=手动, 1=自动循迹 */
static unsigned int  idle_ms = 0;

static signed char   last_err = 0;
static signed int    integ = 0;       /* 积分累计，消除稳态偏差 */
static unsigned char lost_cnt = 0;
static unsigned int  arm_ms = 0;      /* 发 A 后等待速度的窗口 */
static unsigned int  beep_ms = 0;

/* 单轮调试 */
static unsigned char dbg = 0;        /* 1=调试模式 */
static signed char   wl = 0, wr = 0; /* 左右轮直接目标（不走补偿） */

/* ---------------- 工具 ---------------- */
static void send_u8(unsigned char v)
{
    UART_SendByte('0' + v / 100);
    UART_SendByte('0' + (v / 10) % 10);
    UART_SendByte('0' + v % 10);
}

static void send_hex(unsigned char v)
{
    unsigned char h = (unsigned char)(v >> 4);
    UART_SendByte(h < 10 ? ('0' + h) : ('A' + h - 10));
    h = (unsigned char)(v & 0x0F);
    UART_SendByte(h < 10 ? ('0' + h) : ('A' + h - 10));
}

static void beep(unsigned int ms)
{
    beep_ms = ms;
}

/* 右轮速度补偿：把目标值按 trim_r 百分比缩放 */
static signed char trim_wheel(signed char v)
{
    signed int t = (signed int)v * trim_r / 100;
    if (t >  100) t =  100;
    if (t < -100) t = -100;
    return (signed char)t;
}

/* 补偿允许的最高速度：保证补偿后右轮不超过 100%（speed*T/100 <= 100） */
static unsigned char max_speed(void)
{
    unsigned char m = (unsigned char)(10000 / trim_r);
    if (m > 100) m = 100;
    return m;
}

/* 实际生效速度 = min(设定速度, 上限) */
static unsigned char eff_speed(void)
{
    unsigned char m = max_speed();
    return (speed > m) ? m : speed;
}

/* 声光节拍，20ms 一次 */
static void beep_tick(void)
{
    if (beep_ms) {
        if (beep_ms > 20) beep_ms -= 20;
        else beep_ms = 0;
    }
    if (beep_ms) { BUZZER = 0; LED = 0; }
    else         { BUZZER = 1; LED = 1; }
}

/* ---------------- 手动模式 ---------------- */
static void apply_dir(void)
{
    unsigned char sv = eff_speed();
    signed char s = (signed char)sv;
    signed char tl = 0, tr = 0;
    signed char t  = (signed char)(s * TURN_PCT / 100);

    switch (dir) {
    case 'F': tl =  s; tr =  s; break;
    case 'B': tl = -s; tr = -s; break;
    case 'L': tl = -t; tr =  t; break;
    case 'R': tl =  t; tr = -t; break;
    default:  tl = 0;  tr = 0;  dir = 'S'; break;
    }
    tr = trim_wheel(tr);
    Motor_SetTarget(tl, tr);
}

/* ---------------- 自动循迹 ---------------- */
static void track_step(void)
{
    unsigned char on, i;
    signed char sum = 0, cnt = 0, err, steer, d;
    signed int l, r;

#if LINE_ACTIVE_LOW
    on = (unsigned char)(~P2) & 0x7F;      /* 1 = 该探头压到黑线 */
#else
    on = (unsigned char)(P2 & 0x7F);
#endif

    for (i = 0; i < 7; i++) {
        if (on & (1 << i)) {
            sum += (signed char)(i - 3);   /* 权重 -3 ~ +3 */
            cnt++;
        }
    }

    if (cnt == 0) {                         /* 丢线 */
        if (lost_cnt < 50) lost_cnt++;
        if (lost_cnt > 10) Motor_SetTarget(0, 0);   /* 200ms 没找到，停车 */
        return;
    }
    lost_cnt = 0;

    err = (signed char)(sum * 10 / cnt);    /* -30 ~ +30 */
    d   = (signed char)(err - last_err);
    last_err = err;

    integ += err;                           /* 积分：自动补两轮差异 */
    if (integ >  100) integ =  100;
    if (integ < -100) integ = -100;

    steer = (signed char)(err * TRACK_KP / 10 + d * TRACK_KD / 10
                          + integ * TRACK_KI / 10);
    if (steer >  TRACK_MAX) steer =  TRACK_MAX;
    if (steer < -TRACK_MAX) steer = -TRACK_MAX;

    l = (signed int)eff_speed() - steer;
    r = (signed int)eff_speed() + steer;
    r = r * trim_r / 100;                   /* 右轮补偿 */
    if (l >  100) l =  100;
    if (l < -100) l = -100;
    if (r >  100) r =  100;
    if (r < -100) r = -100;
    Motor_SetTarget((signed char)l, (signed char)r);

    if (cnt >= CORNER_ON) beep(BEEP_MS);    /* 直角弯：压线很宽 */
}

/* ---------------- 指令 ---------------- */
static void report(void)
{
    UART_SendString("M=");
    UART_SendByte(dbg ? 'D' : (mode ? 'A' : 'M'));
    UART_SendString(" D=");
    UART_SendByte(dir);
    UART_SendString(" S=");
    send_u8(speed);
    UART_SendString(" T=");
    send_u8(trim_r);
    UART_SendString(" X=");
    send_u8(max_speed());
    UART_SendString(" L=");
#if LINE_ACTIVE_LOW
    send_hex((unsigned char)(~P2) & 0x7F);
#else
    send_hex((unsigned char)(P2) & 0x7F);
#endif
    UART_SendString("\r\n");
}

static unsigned char handle(unsigned char c)
{
    idle_ms = 0;

    /* ---------- 单轮调试模式 ---------- */
    if (dbg) {
        switch (c) {
        case '1': wl =  (signed char)speed; break;   /* 左轮正转 */
        case '2': wl = -(signed char)speed; break;   /* 左轮反转 */
        case '3': wl = 0; break;                     /* 左轮停   */
        case '4': wr =  (signed char)speed; break;   /* 右轮正转 */
        case '5': wr = -(signed char)speed; break;   /* 右轮反转 */
        case '6': wr = 0; break;                     /* 右轮停   */
        case '7': wl =  (signed char)speed; wr = (signed char)speed; break;  /* 都正转 */
        case '8': wl = 0; wr = 0; break;             /* 都停 */
        case '+': if (speed < 90) speed += 5; /* 测试速度 +5 */
                  if (wl > 0) wl = (signed char)speed; else if (wl < 0) wl = -(signed char)speed;
                  if (wr > 0) wr = (signed char)speed; else if (wr < 0) wr = -(signed char)speed;
                  break;
        case '-': if (speed > 10) speed -= 5;
                  if (wl > 0) wl = (signed char)speed; else if (wl < 0) wl = -(signed char)speed;
                  if (wr > 0) wr = (signed char)speed; else if (wr < 0) wr = -(signed char)speed;
                  break;
        case 'D': case 'd':
            dbg = 0; wl = 0; wr = 0;
            Motor_SetRaw(0);
            Motor_Stop();
            UART_SendString("DEBUG OFF\r\n");
            return 0;
        default:
            return 0;
        }
        Motor_SetTarget(wl, wr);
        return 1;
    }

    if (c >= '1' && c <= '9') {
        speed = (unsigned char)((c - '0') * 10);
        if (mode) arm_ms = 0;              /* 发 A2：收到数字立即按新速度起步 */
        else      apply_dir();
        return 1;
    }

    switch (c) {
    case 'F': case 'f':
    case 'B': case 'b':
    case 'L': case 'l':
    case 'R': case 'r':
        if (mode != 0) return 0;
        dir = (char)(c & 0xDF);
        apply_dir();
        return 1;

    case 'S': case 's':
    case ' ': case 'X': case 'x':
        arm_ms = 0;
        mode = 0;
        dir = 'S';
        Motor_Stop();
        return 1;

    case 'A': case 'a':                 /* 切自动循迹：A 或 A2（2=20%） */
        mode = 1;
        speed = TRACK_SPEED;
        last_err = 0;
        integ = 0;
        lost_cnt = 0;
        arm_ms = 400;                   /* 等 0.4 秒，看后面有没有数字 */
        Motor_Stop();
        UART_SendString("AUTO\r\n");
        return 0;

    case 'D': case 'd':                 /* 单轮调试模式 */
        dbg = 1;
        mode = 0;
        wl = 0;
        wr = 0;
        Motor_SetRaw(1);                /* 调试时关闭死区补偿 */
        Motor_Stop();
        UART_SendString("DEBUG ON\r\n");
        return 0;

    case 'M': case 'm':                 /* 切手动 */
        arm_ms = 0;
        mode = 0;
        dir = 'S';
        Motor_Stop();
        UART_SendString("MANUAL\r\n");
        return 0;

    case '?':
        report();
        return 0;

    case '+':                           /* 右轮加速 2% */
        if (trim_r < 180) trim_r += 2;
        if (mode == 0) apply_dir();
        return 1;

    case '-':                           /* 右轮减速 2% */
        if (trim_r > 50) trim_r -= 2;
        if (mode == 0) apply_dir();
        return 1;

    default:
        return 0;
    }
}

void main(void)
{
    unsigned char c;

    UART_Init();
    Motor_Init();
    BUZZER = 1;
    LED = 1;
    UART_SendString("CAR READY\r\n");

    while (1) {
        while (UART_GetChar(&c)) {
#if DEBUG_ECHO
            UART_SendByte(c);
#endif
            if (handle(c)) {
#if ACK_ENABLE
                UART_SendString("OK\r\n");
#endif
            }
        }

        if (flag_20ms) {
            flag_20ms = 0;

            if (mode) {
                if (arm_ms) {               /* 等速度窗口，先不动 */
                    if (arm_ms > 20) arm_ms -= 20;
                    else arm_ms = 0;
                } else {
                    track_step();           /* 自动循迹 */
                }
            }
            Motor_Tick();               /* 速度斜坡 */
            beep_tick();                /* 声光 */

#if LINK_TIMEOUT_MS > 0
            if (mode == 0 && dir != 'S') {
                if (idle_ms < 60000) idle_ms += 20;
                if (idle_ms >= LINK_TIMEOUT_MS) {
                    dir = 'S';
                    Motor_Stop();
                }
            }
#endif
        }
    }
}
