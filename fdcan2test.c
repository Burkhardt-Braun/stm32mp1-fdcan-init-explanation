/*
 * fdcan2test.c - Minimal version for m_can_min_init_to_run()
 */

#include <stdint.h>
#include "stm32mp1xx_ll_rcc.h"

/* ---------- Constants ---------- */
#define ORIGINAL_FDCAN_KER_CK          (74250000UL)
#define FDCAN_KERNEL_MIN_HZ            (80000000UL)
#define ERROR_LINE_FLAG                (0x80000000UL)

#define FDCAN_PLL4_M_DIV  (3U)
#define FDCAN_PLL4_N_MUL  (124U)
#define FDCAN_PLL4_R_DIV  (4U)


extern int printf_local( char* DebugMessage,... );
/* ---------- Register offsets ---------- */
enum m_can_reg {
    M_CAN_CCCR = 0x018,
};

/* ---------- CCCR bit masks ---------- */
#define CCCR_INIT   (1u << 0)
#define CCCR_CCE    (1u << 1)
#define CCCR_CSR    (1u << 4)
#define CCCR_CONFIG_BITS   (CCCR_INIT | CCCR_CCE)

/* ---------- Struct definition (MUST be before inline functions) ---------- */
struct m_can_classdev {
    uint32_t base;
};

/* ---------- MMIO helpers ---------- */
static inline uint32_t mmio_read32(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write32(uint32_t addr, uint32_t value)
{
    *(volatile uint32_t *)addr = value;
}

static inline void udelay_spins(volatile int cycles)
{
    while (cycles--) {
        __asm__ volatile("nop");
    }
}

static inline uint32_t m_can_read(struct m_can_classdev *cdev, enum m_can_reg reg)
{
    return mmio_read32(cdev->base + (uint32_t)reg);
}

static inline void m_can_write(struct m_can_classdev *cdev, enum m_can_reg reg, uint32_t value)
{
    mmio_write32(cdev->base + (uint32_t)reg, value);
}

/* ---------- Core function ---------- */
static void m_can_config_endisable(struct m_can_classdev *cdev, int enable)
{
    uint32_t cccr = m_can_read(cdev, M_CAN_CCCR);
    uint32_t timeout = 100000UL;
    const uint32_t expected_state = enable ? CCCR_CONFIG_BITS : 0U;

    if ((cccr & CCCR_CSR) != 0U) {
        cccr &= ~CCCR_CSR;
    }

    if (enable != 0) {
        m_can_write(cdev, M_CAN_CCCR, cccr | CCCR_INIT);
        udelay_spins(500);
        m_can_write(cdev, M_CAN_CCCR, cccr | CCCR_CONFIG_BITS);
    } else {
        m_can_write(cdev, M_CAN_CCCR, cccr & ~CCCR_CONFIG_BITS);
    }

    while ((m_can_read(cdev, M_CAN_CCCR) & CCCR_CONFIG_BITS) != expected_state) {
        if (timeout == 0U) break;
        timeout--;
        udelay_spins(10);
    }
}

/* ---------- Public function ---------- */
int m_can_min_init_to_run(void)
{
    struct m_can_classdev cdev = { .base = FDCAN2_BASE };
    PLL4_ClocksTypeDef pll4_clocks = {0};

    HAL_Init();// only for demonstration purposes here

    __HAL_RCC_FDCAN_FORCE_RESET();
    __HAL_RCC_FDCAN_RELEASE_RESET();

    __HAL_RCC_FDCAN_CLK_ENABLE();

    //optional check für original fdcan_ker_ck.
    //this might fail but is NOT a serious issue
    HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
    if ( ORIGINAL_FDCAN_KER_CK != pll4_clocks.PLL4_R_Frequency )
    {
        printf_local("Original frequence is %s %lu\n", __FILE__, (uint32_t)__LINE__);
    }
    printf_local("PLL4 source = %lu\n", __HAL_RCC_GET_PLL4_SOURCE());
    printf_local("HSE_VALUE = %lu\n", HSE_VALUE);

    /*
     * Keep this sequence conservative. The exact values are the known-working
     * FDCAN kernel clock setup from the original test code.
     * PLL4_R = HSEVALUE / PLL4_M * PLL4_N / PLL4_R
     * PLL4_R = 24 MHz / 3 * 124 / 4
       = 248 MHz
    */
    LL_RCC_PLL4_Disable();
    LL_RCC_PLL4_SetM(FDCAN_PLL4_M_DIV);
    LL_RCC_PLL4_SetN(FDCAN_PLL4_N_MUL);
    LL_RCC_PLL4_SetR(FDCAN_PLL4_R_DIV);

    LL_RCC_PLL4R_Enable();
    LL_RCC_PLL4_Enable();

    int i=0;
    while (!LL_RCC_PLL4_IsReady() && i++<1000) {//arbitrary
    	__NOP();
    }

    if (!LL_RCC_PLL4_IsReady()) {
    	printf_local("ERROR %s %lu\n", __FILE__, (uint32_t)__LINE__);
        return (int)((uint32_t)__LINE__ | ERROR_LINE_FLAG);
    }

    HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);

    //check for proper frequency
    if (FDCAN_KERNEL_MIN_HZ > pll4_clocks.PLL4_R_Frequency) {
    	printf_local("ERROR %s %lu\n", __FILE__, (uint32_t)__LINE__);
        return (int)((uint32_t)__LINE__ | ERROR_LINE_FLAG);
    }

    m_can_config_endisable(&cdev, 0);

    if ((m_can_read(&cdev, M_CAN_CCCR) & CCCR_INIT) != 0U) {
    	printf_local("INIT still set %s %lu\n", __FILE__, (uint32_t)__LINE__);
        return -11;
    }

    printf_local("OK\n");
    return 0;
}
