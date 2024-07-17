#include "DriverLib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// #include "semphr.h"
// #include "integer.h"
// #include "PollQ.h"
// #include "semtest.h"
// #include "BlockQ.h"
// #include <string.h>

#define ValTempQUEUE_SIZE    (20) // val cola de sensores
//#define mainCHECK_TASK_PRIORITY    (tskIDLE_PRIORITY + 3)
#define SENSOR_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)
#define RECEIVER_TASK_PRIORITY  (tskIDLE_PRIORITY + 2)
#define DISPLAY_TASK_PRIORITY   (tskIDLE_PRIORITY + 3)
#define mainSENSOR_DELAY    ( ( TickType_t ) 100 / portTICK_PERIOD_MS ) // 0.1 segundos -> Frecuencia 10Hz (1/10)
#define mainDISPLAY_DELAY   ( ( TickType_t ) 2000 / portTICK_PERIOD_MS ) // 2 segundos

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


// Definición de la cola que contiene los valores de temperatura
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

  	//OSRAMStringDraw("", 16, 1);
    // Crear las tareas
    xTaskCreate(vTemperatureSensorTask, "TemperatureSensor", configSENSOR_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY,  &xHandleSensorTask);
    xTaskCreate(vReceiverTask, "Filter", configFILTER_STACK_SIZE, NULL, RECEIVER_TASK_PRIORITY, &xHandleReceiverTask);
    xTaskCreate(vDisplayTask, "Display", configDISPLAY_STACK_SIZE, NULL, DISPLAY_TASK_PRIORITY, &xHandleDisplayTask);

	xTaskCreate(vMonitorTask, "Monitor", configMONITOR_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);


    // Iniciar el scheduler de FreeRTOS
    vTaskStartScheduler();

    // El código no debería llegar aquí
    return 0;
}

static void vTemperatureSensorTask(void *pvParameters) {
    //const TickType_t xDelaySensor = mainSENSOR_DELAY;
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
        //xQueueSend(xDisplayQueue, &avg_temp, portMAX_DELAY);
    }
}

static void prvSetupHardware( void )
{
	// Configurar el reloj del sistema para usar el reloj principal
	SysCtlClockSet( SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_6MHZ );

	/* Setup the push button. */
	//SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    //GPIODirModeSet(GPIO_PORTC_BASE, mainPUSH_BUTTON, GPIO_DIR_MODE_IN);
	//GPIOIntTypeSet( GPIO_PORTC_BASE, mainPUSH_BUTTON,GPIO_FALLING_EDGE );
	//IntPrioritySet( INT_GPIOC, configKERNEL_INTERRUPT_PRIORITY );
	//GPIOPinIntEnable( GPIO_PORTC_BASE, mainPUSH_BUTTON );
	//IntEnable( INT_GPIOC );

	// Habilitar el puerto UART0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	//SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);// new new
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);// new new


	/* Set GPIO A0 and A1 as peripheral function.  They are used to output the UART signals. */
	//GPIODirModeSet( GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_DIR_MODE_HW );


	/* Configure the UART for 8-N-1 operation.baudios.8 bits de datos, sin paridad y 1 bit de parada */
	//UARTConfigSet( UART0_BASE, mainBAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE )); // NEW
	UARTConfigSet(UART0_BASE, mainBAUD_RATE,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

  	//UARTIntEnable(UART0_BASE, UART_INT_RX); // NEW
  	//UARTIntEnable(UART0_BASE, UART_INT_TX); // NEW

	/* We don't want to use the fifo.  This is for test purposes to generate as many interrupts as possible. */
	//HWREG( UART0_BASE + UART_O_LCR_H ) &= ~mainFIFO_SET;

	/* Enable Tx interrupts. */
	//HWREG( UART0_BASE + UART_O_IM ) |= UART_INT_TX;  // NEW
    //HWREG( UART0_BASE + UART_O_IM ) |= UART_INT_RX; // NEW

	//IntPrioritySet( INT_UART0, configKERNEL_INTERRUPT_PRIORITY );  // NEW
	//IntEnable( INT_UART0 );  // NEW


    //OSRAMStringDraw("www.FreeRTOS.org", 0, 0);
	//OSRAMStringDraw("LM3S811 demo", 16, 1);
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

	/* What caused the interrupt. */
	ulStatus = UARTIntStatus( UART0_BASE, pdTRUE );

	/* Clear the interrupt. */
	UARTIntClear( UART0_BASE, ulStatus );

	/* Was a Tx interrupt pending? */
	if( ulStatus & UART_INT_TX )
	{
		/* Send the next character in the string.  We are not using the FIFO. */
		if( *pcNextChar != 0 )
		{
			if( !( HWREG( UART0_BASE + UART_O_FR ) & UART_FR_TXFF ) )
			{
				HWREG( UART0_BASE + UART_O_DR ) = *pcNextChar;
			}
			pcNextChar++;
		}
	}
	/* Was a Rx interrupt pending? */
    else if (ulStatus & UART_INT_RX) {
        signed char input;
        portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

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
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
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