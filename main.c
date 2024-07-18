#include "DriverLib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "semphr.h"
#include "integer.h"
#include "PollQ.h"
#include "semtest.h"
#include "BlockQ.h"
#include <string.h>

#define ValTempQUEUE_SIZE    (20) // val cola de sensores
#define SENSOR_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)
#define RECEIVER_TASK_PRIORITY  (tskIDLE_PRIORITY + 2)
#define DISPLAY_TASK_PRIORITY   (tskIDLE_PRIORITY + 3)
#define UART_TASK_PRIORITY (tskIDLE_PRIORITY + 4) // ver valor
#define TOP_TASK_PRIORITY  (tskIDLE_PRIORITY + 5) // ver valor
#define mainSENSOR_DELAY    ( ( TickType_t ) 100 / portTICK_PERIOD_MS ) // 0.1 segundos -> Frecuencia 10Hz (1/10)
#define mainDISPLAY_DELAY   ( ( TickType_t ) 2000 / portTICK_PERIOD_MS ) // 2 segundos
#define mainTOP_DELAY       ( ( TickType_t ) 5000 / portTICK_PERIOD_MS ) // 5 segundos

//#define N 5 // Número de mediciones para el filtro pasa bajos
//#define mainPUSH_BUTTON             GPIO_PIN_4 /* Demo board specifics. */
#define mainBAUD_RATE				( 19200 ) /* UART configuration - note this does not use the FIFO so is not very efficient. */
#define mainFIFO_SET					( 0x10 )
#define timerINTERRUPT_FREQUENCY		( 20000UL )

// Prototipos de las tareas:
//Configure the processor and peripherals for this demo.
static void prvSetupHardware( void );
static void vTemperatureSensorTask(void *pvParameters);
static void vReceiverTask(void *pvParameters);
static void vDisplayTask(void *pvParameters);
//static void vUartTask(void *pvParameters);
static void vTopTask(void *);

static void vMonitorTask(void *pvParameters);

int getN(void);
void setN(int new_N);
void updateArray(int[], int, int);
void reverse(char str[], int length);
char* itoa(int num, char* str, int base);
char* bitMapping(int);
void Timer0IntHandler(void);
unsigned long getTimerTicks(void);
void sendUART0(const char *);


// Definición de las colas
QueueHandle_t xValTempQueue;
QueueHandle_t xValFilterQueue;
//QueueHandle_t xDisplayQueue;

static int N_filter;
static volatile char *pcNextChar;
unsigned long ulHighFrequencyTimerTicks;

// Declaración de los manejadores de las tareas
static TaskHandle_t xHandleSensorTask = NULL;
static TaskHandle_t xHandleReceiverTask = NULL;
static TaskHandle_t xHandleDisplayTask = NULL;
static TaskHandle_t xHandleUartTask = NULL;

#define N_ARRAY    20
#define MAX_TEMP   30
#define MIN_TEMP   10
#define DISPLAY_COLUMNS  96

int main(void)
{
    /* Configure the clocks, UART and GPIO. */
	prvSetupHardware();

    // Inicializar la cola
    xValTempQueue = xQueueCreate(ValTempQUEUE_SIZE, sizeof(int));
    xValFilterQueue = xQueueCreate(ValTempQUEUE_SIZE, sizeof(int));
    //xDisplayQueue = xQueueCreate(ValTempQUEUE_SIZE, sizeof(int));

    setN(10);
    
    /* Initialise the LCD> */
    OSRAMInit( false );

    // Crear las tareas
    xTaskCreate(vTemperatureSensorTask, "Sensor", configSENSOR_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY,  &xHandleSensorTask);
    xTaskCreate(vReceiverTask, "Filter", configFILTER_STACK_SIZE, NULL, RECEIVER_TASK_PRIORITY, &xHandleReceiverTask);
    xTaskCreate(vDisplayTask, "Display", configDISPLAY_STACK_SIZE, NULL, DISPLAY_TASK_PRIORITY, &xHandleDisplayTask);
	//xTaskCreate(vUartTask, "UART", UART_TASK_STACK_SIZE, NULL, UART_TASK_PRIORITY, &xHandleUartTask);
	xTaskCreate(vTopTask, "Top", configTOP_STACK_SIZE, NULL, TOP_TASK_PRIORITY, NULL);

	// Tarea para obtener los valores minimos de stack:
	//xTaskCreate(vMonitorTask, "Monitor", configMONITOR_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);


    // Iniciar el scheduler de FreeRTOS
    vTaskStartScheduler();

    // El código no debería llegar aquí
    return 0;
}

static void vTemperatureSensorTask(void *pvParameters) {
    int temperature_value = MIN_TEMP;
    while (true) {
        /* Sensor gets temperature value */
        if (temperature_value < MAX_TEMP) {
            temperature_value++;
        } else {
            temperature_value = MIN_TEMP;
        }

        xQueueSend(xValTempQueue, &temperature_value, portMAX_DELAY);

        vTaskDelay(mainSENSOR_DELAY); // waits mainSENSOR_DELAY milliseconds for 10Hz frequency
    }
}

static void vReceiverTask(void *pvParameters) {
    int new_temp_value; // value read from sensor
    int avg_temp;

    /* create array to store latest temperature values read */
    int temperature_array[N_ARRAY] = {};

    while (true) {
        xQueueReceive(xValTempQueue, &new_temp_value, portMAX_DELAY);

        /* add new temperature value to array */
        updateArray(temperature_array, N_ARRAY, new_temp_value);

        /* Calculate average */
        int accum = 0;
        for (int i = 0; i < getN(); i++) {
            accum += temperature_array[i];
        }
        avg_temp = accum / getN();
        
        xQueueSend(xValFilterQueue, &avg_temp, portMAX_DELAY);
    }
}

static void prvSetupHardware( void )
{
	/* Enable the UART.  */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	/* Configure the UART for 8-N-1 operation. 
		8 bit word lenght, one stop bit, no parity*/
	UARTConfigSet(UART0_BASE, mainBAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	/* Enable the UART interrupt. */
  	UARTIntEnable(UART0_BASE, UART_INT_RX);
  	//UARTIntEnable(UART0_BASE, UART_INT_TX);

	/* Set priority for UART interrupt. */
	//HWREG( UART0_BASE + UART_O_IM ) |= UART_INT_TX;
	//HWREG( UART0_BASE + UART_O_IM ) |= UART_INT_RX;
	IntPrioritySet(INT_UART0, configKERNEL_INTERRUPT_PRIORITY);
	IntEnable(INT_UART0);
}
/*-----------------------------------------------------------*/

static void vDisplayTask(void *pvParameters) {
	OSRAMStringDraw("SO 2 - TP4", 20, 0);
    // Esperar 2 segundos
    vTaskDelay(mainDISPLAY_DELAY);
    // Limpiar el display
    OSRAMClear();

	/* create array with latest filtered values */
  	int filtered_array[DISPLAY_COLUMNS] = {};

	/* initialize array with minimum temperature value */
  	for (int i = 0 ; i < DISPLAY_COLUMNS; i++) {
    	filtered_array[i] = MIN_TEMP;
  	}

	int new_filtered_value;

	while (true) {
		xQueueReceive(xValFilterQueue, &new_filtered_value, portMAX_DELAY);

		/* stores the conversion int to char */
		char str[2] = {};

		/* update array with new filtered value */
		updateArray(filtered_array, DISPLAY_COLUMNS, new_filtered_value);

		/* clear display */
		OSRAMClear();

		for (int i = DISPLAY_COLUMNS - 1; i > 0; i--) {
			/* draw N of filter value */
			OSRAMStringDraw(itoa(filtered_array[i], str, 10), 0, 1);

			/* draw temperature filtered value */
			OSRAMStringDraw(itoa(getN(), str, 10), 0, 0);

			/* draw graphic on the display */
			int bit_map_half = filtered_array[DISPLAY_COLUMNS - i] >= 20 ? 0 : 1;
			OSRAMImageDraw(bitMapping(filtered_array[DISPLAY_COLUMNS - i]), i+10, bit_map_half , 1, 1);
		}
	}
}

void setN(int new_N) {
    N_filter = new_N;
}

int getN(void) {
    return N_filter;
}

/* Update array with new value and dismiss the oldest value */
void updateArray(int array[], int size, int new_value) {
    for(int i = size - 1; i > 0; i--) {
        array[i] = array[i-1];
    }
    array[0] = new_value;
}

// A utility function to reverse a string
void reverse(char str[], int length){
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        end--;
        start++;
    }
}

/* Convert integer to string: implementation of stdio itoa()*/
char* itoa(int num, char* str, int base){
    int i = 0;
    int isNegative = 0;
 
    /* Handle 0 explicitly, otherwise empty string is
     * printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    // In standard itoa(), negative numbers are handled
    // only with base 10. Otherwise numbers are
    // considered unsigned.
    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }
 
    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }
 
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; // Append string terminator
 
    // Reverse the string
    reverse(str, i);
 
 
    return str;
}


/* Map each temperature value with a pixel in the display */
char* bitMapping(int valor) {
	if ((valor <= 13) || (valor == 20)) {
		return "@";
	}

	if ((valor == 14) || (valor == 21)) {
		return " ";
	}

	if ((valor == 15) || (valor == 22)) {
		return "";
	}

	if ((valor == 16) || (valor == 23)){
		return "";
	}

	if ((valor == 17) || (valor == 24)){
		return "";
	}

	if ((valor == 18) || (valor == 25)) {
		return "";
	}

	if ((valor == 19) || (valor == 26)) {
		return "";
	}
}


void vUART_ISR(void){
	unsigned long ulStatus;
	//sendUART0("\r\ncambio de N\r\n");// sacar
	/* What caused the interrupt. */
	ulStatus = UARTIntStatus( UART0_BASE, pdTRUE );

	/* Clear the interrupt. */
	UARTIntClear( UART0_BASE, ulStatus );

	// /* Was a Tx interrupt pending? */
	// if( ulStatus & UART_INT_TX )
	// {
	// 	/* Send the next character in the string.  We are not using the FIFO. */
	// 	if( *pcNextChar != 0 )
	// 	{
	// 		if( !( HWREG( UART0_BASE + UART_O_FR ) & UART_FR_TXFF ) )
	// 		{
	// 			HWREG( UART0_BASE + UART_O_DR ) = *pcNextChar;
	// 		}
	// 		pcNextChar++;
	// 	}
	// }
	// /* Was a Rx interrupt pending? */
    // else 
	if (ulStatus & UART_INT_RX) {
        signed char input;
        //portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		//sendUART0("\r\ncaracter recibido\r\n"); // sacar
        /* A character was received.  Place it in the queue of received characters. */
        input = UARTCharGet(UART0_BASE);
        if (input == '+' && getN() < N_ARRAY) {
            setN(getN() + 1);
			sendUART0("\r\nN incremented\r\n");
        }
		else if(input == '-' && getN() > 1){
			setN(getN() - 1);
			sendUART0("\r\nN decremented\r\n");
		}
		else {
			sendUART0("\r\nCan't permform that operation on N, limit reached \r\n");
		}
        //portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

/* Send and print string through UART0 */
void sendUART0(const char *string) {
	while (*string != '\0') {
		UARTCharPut(UART0_BASE, *string);
		string++;
	}
}

/* Timer0 set up and configuration */
void setupTimer0(void) {
	/* Enable Timer0 */
  	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
  	TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_TIMER);

	/* Ensure interrupts do not start until the scheduler is running. */
	portDISABLE_INTERRUPTS();

	/* Allow global interrupt */
	IntMasterEnable();
  	
	/* Set Timer0 interrupt */
	unsigned long ulFrequency = configCPU_CLOCK_HZ / timerINTERRUPT_FREQUENCY;
  	TimerLoadSet(TIMER0_BASE, TIMER_A, ulFrequency);
  	TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0IntHandler);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	/* Enable Timer0 */
  	TimerEnable(TIMER0_BASE, TIMER_A);
}

/* Timer0 Interrupt Handler */
void Timer0IntHandler(void) {
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	ulHighFrequencyTimerTicks++;
}

/* Get timer Ticks */
unsigned long getTimerTicks(void) {
  	return ulHighFrequencyTimerTicks;
}


static void vTopTask(void *pvParameters) {
	TaskStatus_t *pxTaskStatusArray;

	/*  obtiene el número total de tareas actualmente activas en el sistema.*/
	UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();

	/* se solicita memoria suficiente para almacenar un array de estructuras */
	pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

	while (true) {
		volatile UBaseType_t uxArraySize, x;
		unsigned long ulTotalRunTime, ulStatsAsPercentage;

		if (pxTaskStatusArray != NULL) {
			/* obtiene la cant de tareas para las cuales hay información sobre su estado. */
			uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );

			/* tiempo total de ejecución de todas las tareas desde el inicio del sistema 
			dividido 100 para calculo de porcentaje*/
			ulTotalRunTime /= 100UL;

			/* Send via UART. */
			sendUART0("\r");

			if (ulTotalRunTime > 0) {/* para evitar divisiones por cero */
				char* current_N[2] = {};
				sendUART0("\r\n\r\n----------- ESTADISTICAS DE TAREAS  -----------\r\n\r\n");
				sendUART0("Filter N: ");
				sendUART0(itoa(getN(), current_N, 10)); // Actual valor de N
				sendUART0("\r\n\r\n");
				sendUART0("Tarea\tTiempo\tCPU%\tStack Libre\r\n");

				/* recorre cada una de las tareas. */
				for (x = 0; x < uxArraySize; x++) {
					/* que porcetaje del tiempo total uso cada tarea. */
					ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / ulTotalRunTime;

					char buffer[10];//8

					/* se envian los nombres de cada tarea */
					sendUART0(pxTaskStatusArray[x].pcTaskName);
					sendUART0("\t");

					/* se envian los tiempos(en ticks) de cada tarea */
					itoa(pxTaskStatusArray[x].ulRunTimeCounter, buffer, 10);
					sendUART0(buffer);
					sendUART0("\t");

					/* Se envia el uso de CPU% de cada tarea */
					if (ulStatsAsPercentage > 0UL) {
						itoa(ulStatsAsPercentage, buffer, 10);
						sendUART0(buffer);
						sendUART0("%\t");
					} else {
						/* If the percentage is zero here then the task has
               			consumed less than 1% of the total run time. */
						sendUART0("<1%\t");
					}


					/* Send Tasks Stack Free */
					/* Closer to 0, closer to overflow */
					itoa(uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle), buffer, 10);
					sendUART0(buffer);
					sendUART0(" words\r\n");
				}
			}
		}

		vTaskDelay(mainTOP_DELAY); // 5 segundos
	}
}

static void vMonitorTask(void *pvParameters) {
    char buffer[50];
    UBaseType_t uxHighWaterMarkSensor, uxHighWaterMarkReceiver, uxHighWaterMarkDisplay;

    for (;;) {
		// Obtén el high water mark de cada tarea
        uxHighWaterMarkSensor = uxTaskGetStackHighWaterMark(xHandleSensorTask);
        uxHighWaterMarkReceiver = uxTaskGetStackHighWaterMark(xHandleReceiverTask);
        uxHighWaterMarkDisplay = uxTaskGetStackHighWaterMark(xHandleDisplayTask);

        // Enviar uso de la pila de la tarea del sensor
        itoa(uxHighWaterMarkSensor, buffer, 10);
        sendUART0("Sensor Task Stack High Water Mark: ");
        sendUART0(buffer);
        sendUART0("\n");

        // Enviar uso de la pila de la tarea del receptor
        itoa(uxHighWaterMarkReceiver, buffer, 10);
        sendUART0("Receiver Task Stack High Water Mark: ");
        sendUART0(buffer);
        sendUART0("\n");

        // Enviar uso de la pila de la tarea de display
        itoa(uxHighWaterMarkDisplay, buffer, 10);
        sendUART0("Display Task Stack High Water Mark: ");
        sendUART0(buffer);
        sendUART0("\n");
		sendUART0("--------------------------------------");
		sendUART0("\n");
        // Espera un tiempo antes de volver a verificar
        vTaskDelay(pdMS_TO_TICKS(10000)); // Verificar cada 10 segundos
    }
}