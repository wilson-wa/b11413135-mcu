#include <stdint.h>

/* ---------------- 暫存器位址定義 ----------------
 * 依 RM0440 記憶體映射：
 *   RCC   基底位址 0x4002 1000（掛在 AHB1）
 *   GPIOA 基底位址 0x4800 0000（掛在 AHB2，每個 GPIO 相隔 0x400）
 * 位址 = 基底 + 偏移
 */
#define RCC_BASE        0x40021000UL
#define GPIOA_BASE      0x48000000UL

/* RCC_AHB2ENR 偏移 +0x4C，控制 AHB2 上各周邊的時脈開關 */
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4CUL))

/* GPIOA 的各暫存器（偏移值見 RM0440 GPIO 章節） */
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_OTYPER    (*(volatile uint32_t *)(GPIOA_BASE + 0x04UL))
#define GPIOA_OSPEEDR   (*(volatile uint32_t *)(GPIOA_BASE + 0x08UL))
#define GPIOA_PUPDR     (*(volatile uint32_t *)(GPIOA_BASE + 0x0CUL))
#define GPIOA_IDR       (*(volatile uint32_t *)(GPIOA_BASE + 0x10UL))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14UL))
#define GPIOA_BSRR      (*(volatile uint32_t *)(GPIOA_BASE + 0x18UL))

#define GPIOAEN         (1UL << 0)     /* RCC_AHB2ENR 的 bit0 = GPIOA 時脈 */
#define LED_PIN         5              /* LD2 接在 PA5 */

/* 空迴圈延遲。reset 後跑內部 HSI 16 MHz，這個數量級約半秒 */
static void delay(volatile uint32_t n)
{
    while (n--) {
        __asm volatile ("nop");        /* 什麼都不做，避免被最佳化掉 */
    }
}

int main(void)
{
    /* ---- 1. 開啟 GPIOA 時脈 ---- */
    /* 周邊時脈預設關閉，沒開就寫不進暫存器（讀回來是 0） */
    RCC_AHB2ENR |= GPIOAEN;
    (void)RCC_AHB2ENR;                          /* dummy read，確保時脈生效 */

    /* ---- 2. PA5 設成一般輸出 ---- */
    /* MODER 每腳 2 位元，PA5 用 bit11:10；00=輸入 01=輸出 10=AF 11=類比 */
    GPIOA_MODER &= ~(3UL << (LED_PIN * 2));     /* 先清除這 2 位元 */
    GPIOA_MODER |=  (1UL << (LED_PIN * 2));     /* 寫入 01 = 輸出 */

    /* ---- 3. 輸出細部設定 ---- */
    GPIOA_OTYPER  &= ~(1UL << LED_PIN);         /* 0 = 推挽輸出 */
    GPIOA_OSPEEDR &= ~(3UL << (LED_PIN * 2));   /* 00 = 低速，點 LED 夠用 */
    GPIOA_PUPDR   &= ~(3UL << (LED_PIN * 2));   /* 00 = 不上拉不下拉 */

    /* ---- 4. 主迴圈：LD2 閃爍 ---- */
    while (1)
    {
        GPIOA_BSRR = (1UL << LED_PIN);          /* 低 16 位元寫 1：PA5=1，LED 亮 */
        delay(400000);

        GPIOA_BSRR = (1UL << (LED_PIN + 16));   /* 高 16 位元寫 1：PA5=0，LED 滅 */
        delay(400000);
    }
}
