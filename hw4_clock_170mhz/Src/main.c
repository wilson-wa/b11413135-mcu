#include <stdint.h>

/* =========================================================
 * W04 hw4：HSI16 → PLL → 170 MHz，並用 SysTick 讓 LD2 以 1 Hz 閃爍自驗
 * 本板 HSE 未焊，PLL 來源必須是 HSI16
 * ========================================================= */

#define REG(addr)       (*(volatile uint32_t *)(addr))

/* ---------- 基底位址（RM0440 記憶體映射） ---------- */
#define PWR_BASE        0x40007000UL
#define FLASH_R_BASE    0x40022000UL
#define RCC_BASE        0x40021000UL
#define GPIOA_BASE      0x48000000UL

/* ---------- RCC ---------- */
#define RCC_CR          REG(RCC_BASE + 0x00UL)
#define RCC_CFGR        REG(RCC_BASE + 0x08UL)
#define RCC_PLLCFGR     REG(RCC_BASE + 0x0CUL)
#define RCC_AHB2ENR     REG(RCC_BASE + 0x4CUL)
#define RCC_APB1ENR1    REG(RCC_BASE + 0x58UL)

#define RCC_CR_PLLON        (1UL << 24)
#define RCC_CR_PLLRDY       (1UL << 25)

#define RCC_CFGR_SW_MASK    (3UL << 0)
#define RCC_CFGR_SW_PLL     (3UL << 0)     /* 11 = PLL */
#define RCC_CFGR_SWS_MASK   (3UL << 2)
#define RCC_CFGR_SWS_PLL    (3UL << 2)     /* 11 = 目前真的是 PLL */
#define RCC_CFGR_HPRE_MASK  (0xFUL << 4)
#define RCC_CFGR_HPRE_DIV2  (0x8UL << 4)   /* AHB ÷2 */

#define PLLCFGR_SRC_HSI16   (2UL << 0)     /* PLLSRC = 10：HSI16 */
#define PLLCFGR_M_DIV4      (3UL << 4)     /* 欄位存「除數-1」：3 → ÷4 */
#define PLLCFGR_N_X85       (85UL << 8)    /* PLLN = 85 */
#define PLLCFGR_R_DIV2      (0UL << 25)    /* PLLR = 00 → ÷2 */
#define PLLCFGR_REN         (1UL << 24)    /* PLL R 輸出致能 */

#define RCC_APB1ENR1_PWREN  (1UL << 28)
#define RCC_AHB2ENR_GPIOAEN (1UL << 0)

/* ---------- PWR ---------- */
#define PWR_CR5             REG(PWR_BASE + 0x80UL)
#define PWR_CR5_R1MODE      (1UL << 8)     /* 1 = 一般 Range 1；清 0 = boost */

/* ---------- FLASH ---------- */
#define FLASH_ACR           REG(FLASH_R_BASE + 0x00UL)
#define FLASH_ACR_LATENCY_MASK  (0xFUL << 0)
#define FLASH_ACR_LATENCY_4WS   (4UL << 0)

/* ---------- GPIOA ---------- */
#define GPIOA_MODER     REG(GPIOA_BASE + 0x00UL)
#define GPIOA_BSRR      REG(GPIOA_BASE + 0x18UL)
#define LED_PIN         5

/* ---------- SysTick（Cortex-M 核心私有區 0xE000 xxxx） ---------- */
#define SYST_CSR        REG(0xE000E010UL)
#define SYST_RVR        REG(0xE000E014UL)
#define SYST_CVR        REG(0xE000E018UL)
#define SYST_CSR_ENABLE     (1UL << 0)
#define SYST_CSR_CLKSOURCE  (1UL << 2)     /* 1 = 使用處理器時脈 */
#define SYST_CSR_COUNTFLAG  (1UL << 16)    /* 數到 0 時為 1，讀取後自動清除 */

uint32_t SystemCoreClock = 16000000UL;     /* reset 後是 HSI16 */

/* 短暫等待（用在切換後等待 ≥1 µs） */
static void short_wait(volatile uint32_t n)
{
    while (n--) {
        __asm volatile ("nop");
    }
}

static void clock_init_170mhz(void)
{
    /* 0. 開 PWR 時脈：沒開時脈，寫 PWR_CR5 沒有作用 */
    RCC_APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC_APB1ENR1;

    /* 0.5 先把 AHB 設成 ÷2：升到 80 MHz 以上的過渡期間避免電流突增 */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_HPRE_MASK) | RCC_CFGR_HPRE_DIV2;

    /* 1. 進 boost mode：清 R1MODE（清 0 才是 boost，方向別記反） */
    PWR_CR5 &= ~PWR_CR5_R1MODE;

    /* 2. Flash 等待週期設 4，並讀回確認生效 */
    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) | FLASH_ACR_LATENCY_4WS;
    while ((FLASH_ACR & FLASH_ACR_LATENCY_MASK) != FLASH_ACR_LATENCY_4WS) {
    }

    /* 3. PLL 參數：HSI16 ÷4 ×85 ÷2 = 170 MHz（VCO = 340 MHz，在 96–344 內） */
    RCC_PLLCFGR = PLLCFGR_SRC_HSI16 | PLLCFGR_M_DIV4 | PLLCFGR_N_X85
                | PLLCFGR_R_DIV2 | PLLCFGR_REN;

    /* 4. 開 PLL，等負回授鎖定 */
    RCC_CR |= RCC_CR_PLLON;
    while ((RCC_CR & RCC_CR_PLLRDY) == 0) {
    }

    /* 5. 切換 SYSCLK 到 PLL，並確認真的切過去 */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
    }

    /* 5.5 等 ≥1 µs 後，AHB 恢復 ÷1（此時 HCLK 85 MHz，200 圈遠超過 1 µs） */
    short_wait(200);
    RCC_CFGR &= ~RCC_CFGR_HPRE_MASK;

    /* 6. 收尾：更新軟體記錄的時脈 */
    SystemCoreClock = 170000000UL;
}

/* SysTick 設成每 1 ms 數到 0 一次（輪詢，不用中斷） */
static void systick_init_1ms(void)
{
    SYST_RVR = (SystemCoreClock / 1000UL) - 1UL;   /* 170 MHz → 169999 */
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_ENABLE;
}

static void delay_ms(uint32_t ms)
{
    while (ms--) {
        while ((SYST_CSR & SYST_CSR_COUNTFLAG) == 0) {
        }
    }
}

int main(void)
{
    clock_init_170mhz();
    systick_init_1ms();

    /* LD2（PA5）設成輸出，和 hw3 相同 */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC_AHB2ENR;
    GPIOA_MODER = (GPIOA_MODER & ~(3UL << (LED_PIN * 2)))
                | (1UL << (LED_PIN * 2));

    /* 1 Hz 閃爍：亮 500 ms、滅 500 ms → 手機碼表 30 秒應數到 30 次 */
    while (1) {
        GPIOA_BSRR = (1UL << LED_PIN);
        delay_ms(500);
        GPIOA_BSRR = (1UL << (LED_PIN + 16));
        delay_ms(500);
    }
}
