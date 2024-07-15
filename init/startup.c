void ResetISR(void);
static void NmiSR(void);
static void FaultISR(void);
static void IntDefaultHandler(void);

// ISR Handlers
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
extern void vPortSVCHandler(void);

// Default ISR Handlers
//void vGPIO_ISR(void) __attribute__((weak, alias("IntDefaultHandler"))); // DISTINTO
//void vUART_ISR(void) __attribute__((weak, alias("IntDefaultHandler"))); // DISTINTO
extern void vGPIO_ISR(void);
extern void vUART_ISR( void);

extern int main(void);

#ifndef STACK_SIZE
#define STACK_SIZE 64
#endif

static unsigned long pulStack[STACK_SIZE];

__attribute__((section(".isr_vector")))
void (*const g_pfnVectors[])(void) =
{
    (void (*)(void))((unsigned long)pulStack + sizeof(pulStack)),
    ResetISR,
    NmiSR,
    FaultISR,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    0,
    0,
    0,
    0,
    vPortSVCHandler,
    IntDefaultHandler,
    0,
    xPortPendSVHandler,
    xPortSysTickHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    vUART_ISR,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler,
    IntDefaultHandler
};

extern unsigned long _etext;
extern unsigned long _data;
extern unsigned long _edata;
extern unsigned long _bss;
extern unsigned long _ebss;

void ResetISR(void)
{
    unsigned long *pulSrc, *pulDest;

    // pulSrc = &_etext;
    // for (pulDest = &_data; pulDest < &_edata;)
    // {
    //     *pulDest++ = *pulSrc++;
    // }

    // for (pulDest = &_bss; pulDest < &_ebss;)
    // {
    //     *pulDest++ = 0;
    // }

    main();
}

static void NmiSR(void)
{
    while (1)
    {
    }
}

static void FaultISR(void)
{
    while (1)
    {
    }
}

static void IntDefaultHandler(void)
{
    while (1)
    {
    }
}
