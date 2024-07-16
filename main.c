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
#define mainCHECK_TASK_PRIORITY    (tskIDLE_PRIORITY + 3)
#define mainCHECK_DELAY    ( ( TickType_t ) 100 / portTICK_PERIOD_MS ) // 0.1 segundos -> Frecuencia 10Hz (1/10)
//#define N 5 // Número de mediciones para el filtro pasa bajos
//#define mainPUSH_BUTTON             GPIO_PIN_4 /* Demo board specifics. */
#define mainBAUD_RATE				( 19200 ) /* UART configuration - note this does not use the FIFO so is not very efficient. */
#define mainFIFO_SET					( 0x10 )

// Prototipos de las tareas:
//Configure the processor and peripherals for this demo.
static void prvSetupHardware( void );
static void vTemperatureSensorTask(void *pvParameters);
static void vReceiverTask(void *pvParameters);
static void vDisplayTask(void *pvParameters);

int getN(void);
void setN(int new_N);
void updateArray(int[], int, int);
void reverse(char str[], int length);
char* itoa(int num, char* str, int base);
char* bitMapping(int);

// Definición de la cola que contiene los valores de temperatura
QueueHandle_t xValTempQueue;
QueueHandle_t xValFilterQueue;
//QueueHandle_t xDisplayQueue;

static int N_filter;

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
    xTaskCreate(vTemperatureSensorTask, "TemperatureSensor", configSENSOR_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY - 2, NULL);
    xTaskCreate(vReceiverTask, "Filter", configFILTER_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY - 2, NULL);
    xTaskCreate(vDisplayTask, "Display", configDISPLAY_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY - 2, NULL);

    // Iniciar el scheduler de FreeRTOS
    vTaskStartScheduler();

    // El código no debería llegar aquí
    return 0;
}

static void vTemperatureSensorTask(void *pvParameters) {
    const TickType_t xDelaySensor = mainCHECK_DELAY;
    int temperature_value = MIN_TEMP;
    while (true) {
        /* Sensor gets temperature value */
        if (temperature_value < MAX_TEMP) {
            temperature_value++;
        } else {
            temperature_value = MIN_TEMP;
        }

        xQueueSend(xValTempQueue, &temperature_value, portMAX_DELAY);

        vTaskDelay(xDelaySensor); // waits mainCHECK_DELAY milliseconds for 10Hz frequency
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
	/* Setup the PLL. */
	//SysCtlClockSet( SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_6MHZ );

	/* Setup the push button. */
	//SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    //GPIODirModeSet(GPIO_PORTC_BASE, mainPUSH_BUTTON, GPIO_DIR_MODE_IN);
	//GPIOIntTypeSet( GPIO_PORTC_BASE, mainPUSH_BUTTON,GPIO_FALLING_EDGE );
	//IntPrioritySet( INT_GPIOC, configKERNEL_INTERRUPT_PRIORITY );
	//GPIOPinIntEnable( GPIO_PORTC_BASE, mainPUSH_BUTTON );
	//IntEnable( INT_GPIOC );

	/* Enable the UART.  */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	//SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	/* Set GPIO A0 and A1 as peripheral function.  They are used to output the UART signals. */
	//GPIODirModeSet( GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_DIR_MODE_HW );

	/* Configure the UART for 8-N-1 operation. */
	UARTConfigSet( UART0_BASE, mainBAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE ));


  	UARTIntEnable(UART0_BASE, UART_INT_RX); // NEW
  	UARTIntEnable(UART0_BASE, UART_INT_TX); // NEW
	/* We don't want to use the fifo.  This is for test purposes to generate as many interrupts as possible. */
	//HWREG( UART0_BASE + UART_O_LCR_H ) &= ~mainFIFO_SET;

	/* Enable Tx interrupts. */
	HWREG( UART0_BASE + UART_O_IM ) |= UART_INT_TX;
    HWREG( UART0_BASE + UART_O_IM ) |= UART_INT_RX; // NEW

	IntPrioritySet( INT_UART0, configKERNEL_INTERRUPT_PRIORITY );
	IntEnable( INT_UART0 );


    //OSRAMStringDraw("www.FreeRTOS.org", 0, 0);
	//OSRAMStringDraw("LM3S811 demo", 16, 1);
}
/*-----------------------------------------------------------*/

static void vDisplayTask(void *pvParameters) {
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


void vUART_ISR(void) {
    // ISR de UART
}

void setupTimer0(void) {
    // Configuración del Timer0
}

unsigned long getTimerTicks(void) {
    // Obtener ticks del timer
    return 0; // ulHighFrequencyTimerTicks;
}
