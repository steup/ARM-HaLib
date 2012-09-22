#include <stdint.h>
#include "arm-halib/arm/exceptions.h"

typedef void* StackPtr;

typedef void(*InitFunc)(void);
typedef void(*FiniFunc)(void);

extern "C"
{
    extern uint8_t __end;
    extern uint8_t __text_end;
    extern uint32_t __data_start;
    extern uint32_t __data_end;
    extern uint32_t __data_load_start;
    extern uint32_t __data_load_end;
    extern uint32_t __bss_start;
    extern uint32_t __bss_end;
    extern uint8_t __start;
    extern void* __init_array_start;
    extern void* __init_array_end;
    extern void* __fini_array_start;
    extern void* __fini_array_end;

    int main(void);
}

typedef volatile uint32_t Register;

static const char*    pmc           = (char*)0x400E0400U;
static const uint32_t morOffset     = 0x20;
static const uint32_t mckrOffset    = 0x30;
static const uint32_t pllAOffset    = 0x28;
static const uint32_t srOffset      = 0x68;
static const uint32_t cssMask       = 0x3 << 0;
static const uint32_t cssMain       = 0x1 << 0;
static const uint32_t cssPllA       = 0x2 << 0;
static const uint32_t pres2         = 0x1 << 4;
static const uint32_t masterReady   = 0x1 << 3;
static const uint32_t lockA         = 0x1 << 1;
static const uint32_t pllOne        = 0x1 << 29;
static const uint8_t  pllMulShift   = 16;
static const uint32_t pllMulMask    = 0x7ff << pllMulShift;
static const uint8_t  pllCountShift = 8;
static const uint32_t pllCountMask  = 0x3f << pllCountShift;
static const uint8_t  pllDivShift   = 0;
static const uint32_t pllDivMask    = 0xff << pllDivShift;
static const uint8_t  xtalWaitShift = 8;
static const uint32_t selectXtal    = 1<<24;
static const uint32_t xtalStable    = 1<<0;
static const uint32_t rcStable    = 1<<1;
static const uint32_t enableXtal    = 1<<0;
static const uint32_t enableRC      = 1<<3;
static const uint32_t key           = 0x37<<16;
static const uint32_t selInProgress = 1<<16;
static const uint32_t rc12MHz       = 0x2<<4;

extern "C"
{
    void init(void)                     __attribute__((section(".init"),naked));
    inline void clockSetup(void)        __attribute__((always_inline));
    inline void copyData(void)          __attribute__((always_inline));
    inline void clearBSS(void)          __attribute__((always_inline));
    inline void initArray(void)         __attribute__((always_inline));
    inline void enterMain(void)         __attribute__((always_inline));

    inline void finiArray(void)         __attribute__((always_inline));
    inline void disableInterrupts(void) __attribute__((always_inline));
    inline void loop(void)              __attribute__((always_inline));

    void clockSetup(void)
    {
        Register* mor  = (Register*)(pmc + morOffset);
        Register* mckr = (Register*)(pmc + mckrOffset);
        Register* plla = (Register*)(pmc + pllAOffset);
        Register* sr   = (Register*)(pmc + srOffset);

        static const uint32_t pllConfig    = pllOne | ((PLL_MUL-1) << pllMulShift) | (1 << pllCountShift) | (PLL_DIV << pllDivShift);
        static const uint32_t masterConfig = pres2 | cssPllA;
        static const uint32_t xtalWaitCount = 8 << xtalWaitShift;

        *mor = key | enableRC | enableXtal | xtalWaitCount;
        while (!(*sr & xtalStable));

        *mor = key | xtalWaitCount | enableRC | enableXtal | selectXtal;
        while (!(*sr & selInProgress));

        *plla = pllConfig;
        while(!(*sr & lockA));

        *mckr = (masterConfig & ~cssMask) | cssMain;
        while(!(*sr & masterReady));
        
        *mckr = masterConfig;
        while(!(*sr & masterReady));
        
    }

    void copyData(void)
    {
        uint32_t* srcPtr=&__data_start;
        uint32_t* dstPtr=&__data_load_start;
        while(srcPtr<&__data_end)
            *srcPtr++ = *dstPtr++;
    }

    void clearBSS(void)
    {
        uint32_t *ptr=&__bss_start;
        while(ptr<&__bss_end)
            *ptr++ = 0;  
    }
    void initArray(void)
    {
        for(void **ptr = &__init_array_start;
                   ptr < &__init_array_end;
                   ptr++
           )
            ((InitFunc)*ptr)();

    }
    void enterMain(void)
    {
        main();
    }

    void finiArray(void)
    {
        for(void **ptr = &__fini_array_start;
                   ptr < &__fini_array_end;
                   ptr++
            )
            ((FiniFunc)*ptr)();

    }
    void disableInterrupts(void)
    {
        asm volatile ("cpsid f");
    }
    void loop(void)
    {
        while(1);
    }

    void init()
    {
        disableInterrupts();
        clockSetup();
        copyData();
        clearBSS();
        initArray();
        enterMain();
        finiArray();
        disableInterrupts();
        loop();
    }
    
    StackPtr tos __attribute__((section(".tos"))) = (StackPtr)&__end;

    VecFunc exceptionHandlers[] __attribute__ ((section(".vectors"))) = {

        init/*,
        NMI_Handler,
        HardFault_Handler,
        MemManage_Handler,
        BusFault_Handler,
        UsageFault_Handler,
        0,
        0, 
        0, 
        0,
        SVC_Handler,
        DebugMon_Handler,
        0,
        PendSV_Handler,
        SysTick_Handler,
        SUPC_Handler,
        RSTC_Handler,
        RTC_Handler,
        RTT_Handler,
        WDT_Handler,
        PMC_Handler,
        EFC_Handler,
        0,
        UART0_Handler,
        UART1_Handler,
        SMC_Handler,
        PIOA_Handler,
        PIOB_Handler,
        PIOC_Handler,
        USART0_Handler,
        USART1_Handler,
        0,
        0,
        HSMCI_Handler,
        TWI0_Handler,
        TWI1_Handler,
        SPI_Handler,
        SSC_Handler,
        TC0_Handler,
        TC1_Handler,
        TC2_Handler,
        TC3_Handler,
        TC4_Handler,
        TC5_Handler,
        ADC_Handler,
        DACC_Handler,
        PWM_Handler,
        CRCCU_Handler,
        ACC_Handler,
        UDP_Handler,
        0*/
    };
}
