#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION        1
#define configUSE_IDLE_HOOK         0
#define configUSE_TICK_HOOK         0
#define configCPU_CLOCK_HZ          ( ( unsigned long ) 50000000 ) // DISTINTO
#define configTICK_RATE_HZ          ( ( TickType_t ) 1000 )
//#define configMINIMAL_STACK_SIZE    ( ( unsigned short ) 70 )
//#define configTOTAL_HEAP_SIZE       ( ( size_t ) ( 10240 ) )
#define configMINIMAL_STACK_SIZE    ( ( unsigned short ) 70 )
#define configTOTAL_HEAP_SIZE       ( ( size_t ) ( 4096 ) )
#define configMAX_TASK_NAME_LEN     ( 10 )
#define configUSE_TRACE_FACILITY    1   //CAMBIADO
#define configUSE_16_BIT_TICKS      0
#define configIDLE_SHOULD_YIELD     0   //CAMBIADO

#define configMAX_PRIORITIES        ( 5 )

// stack sizes for each task
#define configSENSOR_STACK_SIZE ((unsigned short) (38))
#define configFILTER_STACK_SIZE ((unsigned short) (62))
#define configDISPLAY_STACK_SIZE ((unsigned short) (145))
#define configTOP_STACK_SIZE ((unsigned short) (56))

#define configGENERATE_RUN_TIME_STATS 1 //NEW
#define configSUPPORT_DYNAMIC_ALLOCATION 1//NEW

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() (setupTimer0())
#define portGET_RUN_TIME_COUNTER_VALUE() (getTimerTicks())

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet        0
#define INCLUDE_uxTaskPriorityGet       0
#define INCLUDE_vTaskDelete             0 //CAMBIADO
#define INCLUDE_vTaskCleanUpResources   0
#define INCLUDE_vTaskSuspend            0 //CAMBIADO
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelay              1
#define INCLUDE_uxTaskGetStackHighWaterMark 1 //new

#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191 /* equivalent to 0xa0, or priority 5. */

#endif /* FREERTOS_CONFIG_H */
