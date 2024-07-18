#include "DriverLib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define SENSOR_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)
#define RECEIVER_TASK_PRIORITY  (tskIDLE_PRIORITY + 2)
#define DISPLAY_TASK_PRIORITY   (tskIDLE_PRIORITY + 3)
#define TOP_TASK_PRIORITY  		(tskIDLE_PRIORITY + 1) 

#define mainSENSOR_DELAY    	( ( TickType_t ) 100 / portTICK_PERIOD_MS ) // 0.1 segundos -> Frecuencia 10Hz (1/10)
#define mainDISPLAY_DELAY  		( ( TickType_t ) 2000 / portTICK_PERIOD_MS ) // 2 segundos
#define mainTOP_DELAY      		( ( TickType_t ) 5000 / portTICK_PERIOD_MS ) // 5 segundos
#define mainMONITOR_DELAY      		( ( TickType_t ) 10000 / portTICK_PERIOD_MS ) // 10 segundos

#define mainBAUD_RATE				( 19200 ) 
#define timerINTERRUPT_FREQUENCY	( 20000UL )

// Prototipos de las tareas
static void prvSetupHardware( void );
static void vTemperatureSensorTask(void *pvParameters);
static void vReceiverTask(void *pvParameters);
static void vDisplayTask(void *pvParameters);
static void vTopTask(void *);
static void vMonitorTask(void *pvParameters);

// Funciones generales
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

// Variables globales
static int N_filter;
unsigned long ulHighFrequencyTimerTicks;

// Declaración de los manejadores de las tareas
static TaskHandle_t xHandleSensorTask = NULL;
static TaskHandle_t xHandleReceiverTask = NULL;
static TaskHandle_t xHandleDisplayTask = NULL;

// Constantes
#define DISPLAY_COLUMNS  96
#define MAX_TEMP   30
#define MIN_TEMP   10
#define N_ARRAY    (MAX_TEMP-MIN_TEMP)				 // tamaño del arreglo de temperaturas
#define VAL_MEDIO (MAX_TEMP-MIN_TEMP)				 // valor de la mitad de la pantalla
#define ValTempQUEUE_SIZE    ((MAX_TEMP-MIN_TEMP))   // tamaño de la cola de sensor

int main(void)
{
    // Configura hardware
	prvSetupHardware();

    // Inicializar la cola
    xValTempQueue = xQueueCreate(ValTempQUEUE_SIZE, sizeof(int));
    xValFilterQueue = xQueueCreate(ValTempQUEUE_SIZE, sizeof(int));

	// N del filtro
    setN(10);
    
    // Inicializa LCD
    OSRAMInit( false );

    // Crea las tareas
    xTaskCreate(vTemperatureSensorTask, "Sensor", configSENSOR_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY,  &xHandleSensorTask);
    xTaskCreate(vReceiverTask, "Filter", configFILTER_STACK_SIZE, NULL, RECEIVER_TASK_PRIORITY, &xHandleReceiverTask);
    xTaskCreate(vDisplayTask, "Display", configDISPLAY_STACK_SIZE, NULL, DISPLAY_TASK_PRIORITY, &xHandleDisplayTask);
	xTaskCreate(vTopTask, "Top", configTOP_STACK_SIZE, NULL, TOP_TASK_PRIORITY, NULL);

	// Tarea para obtener los valores minimos de stack:
	//xTaskCreate(vMonitorTask, "Monitor", configMONITOR_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    // Inicia el scheduler de FreeRTOS
    vTaskStartScheduler();

    return 0;
}

/**
 * @brief Tarea del sensor de temperatura.
 * 
 * Esta tarea simula un sensor de temperatura generando valores dentro de un rango(MIN_TEMP y MAX_TEMP)
 * y los envía a una cola.
 * 
 * @param pvParameters Parámetro de la tarea (no utilizado).
 */

static void vTemperatureSensorTask(void *pvParameters) {
    int temperature_value = MIN_TEMP;
    while (true) {
        if (temperature_value < MAX_TEMP) {
            temperature_value++;
        } else {
            temperature_value = MIN_TEMP;
        }

        xQueueSend(xValTempQueue, &temperature_value, portMAX_DELAY);

        vTaskDelay(mainSENSOR_DELAY); 
    }
}

/**
 * @brief Tarea del receptor.
 * 
 * Esta tarea recibe valores de temperatura de la cola del sensor,
 * aplica un filtro de paso bajo y envía los valores filtrados a otra cola.
 * 
 * @param pvParameters Parámetro de la tarea (no utilizado).
 */
static void vReceiverTask(void *pvParameters) {
    int new_temp_value; // valor leido del sensor
    int avg_temp;

	// Crea un arreglo para almacenar los últimos valores de temperatura leídos
    int temperature_array[N_ARRAY] = {};

    while (true) {
        xQueueReceive(xValTempQueue, &new_temp_value, portMAX_DELAY);

		// Agrega nuevo valor de temperatura a la matriz
        updateArray(temperature_array, N_ARRAY, new_temp_value);

        // Calcula promedio
        int accum = 0;
        for (int i = 0; i < getN(); i++) {
            accum += temperature_array[i];
        }
        avg_temp = accum / getN();
        
        xQueueSend(xValFilterQueue, &avg_temp, portMAX_DELAY);
    }
}

/**
 * @brief Configura el hardware.
 * 
 * Inicializa UART0 y su interrupción, y configura la UART para la operación 8-N-1.
 */
static void prvSetupHardware( void )
{
	//Habilita UART
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	// Configurar UART para operación 8-N-1. Longitud de palabra de 8 bits, un bit de parada, sin paridad
	UARTConfigSet(UART0_BASE, mainBAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	// Habilita la interrupción UART
  	UARTIntEnable(UART0_BASE, UART_INT_RX);

	// Establece la prioridad para la interrupción UART.
	IntPrioritySet(INT_UART0, configKERNEL_INTERRUPT_PRIORITY);
	IntEnable(INT_UART0);
}

/**
 * @brief Tarea de display.
 * 
 * Esta tarea recibe valores filtrados de temperatura y los muestra en una pantalla LCD.
 * 
 * @param pvParameters Parámetro de la tarea (no utilizado).
 */
static void vDisplayTask(void *pvParameters) {
	OSRAMStringDraw("SO 2 - TP4", 18, 0);
    // Esperar 2 segundos
    vTaskDelay(mainDISPLAY_DELAY);

	// crea el array con los últimos valores filtrados 
  	int filtered_array[DISPLAY_COLUMNS] = {};

	// inicializa el array con el valor mínimo de temperatura
  	for (int i = 0 ; i < DISPLAY_COLUMNS; i++) {
    	filtered_array[i] = MIN_TEMP;
  	}

	int new_filtered_value;

	while (true) {
		xQueueReceive(xValFilterQueue, &new_filtered_value, portMAX_DELAY);

		// almacena la conversión de int a char 
		char str[2] = {};

		// actualiza el arrat con el nuevo valor filtrado
		updateArray(filtered_array, DISPLAY_COLUMNS, new_filtered_value);

		// limpia la pantalla
		OSRAMClear();

		for (int i = DISPLAY_COLUMNS - 1; i > 0; i--) {
			// Dibuja N valor del filtro en bloque superior
			OSRAMStringDraw(itoa(getN(), str, 10), 0, 0);

			// dibuja valor filtrado de temperatura de extracción en bloque inferior
			OSRAMStringDraw(itoa(filtered_array[i], str, 10), 0, 1);

			// dibuja el gráfico en la pantalla 
			int bit_map_half = filtered_array[DISPLAY_COLUMNS - i] >= VAL_MEDIO ? 0 : 1;
			OSRAMImageDraw(bitMapping(filtered_array[DISPLAY_COLUMNS - i]), i+11, bit_map_half , 1, 1);
		}
	}
}

/**
 * @brief Configura el valor de N para el filtro.
 * 
 * @param new_N Nuevo valor de N.
 */
void setN(int new_N) {
    N_filter = new_N;
}

/**
 * @brief Obtiene el valor actual de N.
 * 
 * @return int El valor actual de N.
 */
int getN(void) {
    return N_filter;
}

/**
 * @brief Actualiza el array con el nuevo valor y descarta el valor más antiguo.
 * 
 * @param array Array de valores de temperatura.
 * @param size Tamaño del array.
 * @param new_value Nuevo valor a agregar al array.
 */
void updateArray(int array[], int size, int new_value) {
    for(int i = size - 1; i > 0; i--) {
        array[i] = array[i-1];
    }
    array[0] = new_value;
}

/**
 * @brief Invierte una cadena de caracteres.
 * 
 * @param str Cadena de caracteres a invertir.
 * @param length Longitud de la cadena.
 */
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

/**
 * @brief Convierte un número entero a una cadena de caracteres.
 * 
 * @param num Número entero a convertir.
 * @param str Cadena de caracteres resultante.
 * @param base Base para la conversión.
 * @return char* La cadena de caracteres resultante.
 */
char* itoa(int num, char* str, int base){
    int i = 0;
    int isNegative = 0;
 
	// Maneja 0 explícitamente; de ​​lo contrario, se imprime una cadena vacía para 0
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
	// En itoa() estándar, los números negativos se manejan sólo con base 10. De lo contrario, los números se consideran sin signo.
    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }
 
    // Procesa dígitos individuales
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }
 
    // Si el número es negativo, agrega '-'
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; // Agregar terminador de cadena
 
    // Invertir la cadena
    reverse(str, i);
 

    return str;
}


/**
 * @brief Mapea un valor de temperatura a un patrón de bits.
 * 
 * @param valor Valor de temperatura.
 * @return char* Patrón de bits correspondiente.
 */
char* bitMapping(int valor) {
	if ((valor <= 13) || (valor == 20)) {
		return "\100"; // 01000000
	}

	if ((valor == 14) || (valor == 21)) {
		return " ";
	}

	if ((valor == 15) || (valor == 22)) {
		return "\020"; // 00010000
	}

	if ((valor == 16) || (valor == 23)){
		return "\010"; // 00001000
	}

	if ((valor == 17) || (valor == 24)){
		return "\004"; // 00000100
	}

	if ((valor == 18) || (valor == 25)) {
		return "\002"; // 00000010
	}

	if ((valor == 19) || (valor == 26)) {
		return "\001"; // 00000001
	}
}

/**
 * @brief Manejador de interrupción de UART.
 * 
 * Este manejador se ejecuta cuando hay una interrupción en UART.
 * Lee el carácter recibido y ajusta el valor de N según la entrada ('+' para incrementar, '-' para decrementar).
 */
void vUART_ISR(void){
	unsigned long ulStatus;

	// Que causo la interrupcion
	ulStatus = UARTIntStatus( UART0_BASE, pdTRUE );

	// Limpia la interrupcion
	UARTIntClear( UART0_BASE, ulStatus );

	if (ulStatus & UART_INT_RX) {
        signed char input;
		// Caracter recibido
        input = UARTCharGet(UART0_BASE);
        if (input == '+' && getN() < N_ARRAY) {
            setN(getN() + 1);
			sendUART0("\r\nN incrementado\r\n");
        }
		else if(input == '-' && getN() > 1){
			setN(getN() - 1);
			sendUART0("\r\nN decrementado\r\n");
		}
		else {
			sendUART0("\r\nNO se puede realizar esa operacion sobre N. Limite alcanzado \r\n");
		}
    }
}

/**
 * @brief Enviar una cadena de caracteres a través de UART0.
 * 
 * @param string Cadena de caracteres a enviar.
 */
void sendUART0(const char *string) {
	while (*string != '\0') {
		UARTCharPut(UART0_BASE, *string);
		string++;
	}
}

/**
 * @brief Configuración del Timer0.
 * 
 * Configura el Timer0 para generar interrupciones periódicas.
 * Esta función inicializa el Timer0, configura su interrupción y lo habilita.
 */
void setupTimer0(void) {
	// Habilita Timer0
  	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
  	TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_TIMER);

	// Asegura de que las interrupciones no comiencen hasta que se esté ejecutando el programador
	portDISABLE_INTERRUPTS();

	// Permite las interrupciones globales
	IntMasterEnable();
  	
	// Setea interrupcion por Timer0
	unsigned long ulFrequency = configCPU_CLOCK_HZ / timerINTERRUPT_FREQUENCY;
  	TimerLoadSet(TIMER0_BASE, TIMER_A, ulFrequency);
  	TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0IntHandler);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	//Habilita Timer0
  	TimerEnable(TIMER0_BASE, TIMER_A);
}

/**
 * @brief Manejador de la interrupción del Timer0.
 * 
 * Incrementa el contador de ticks de alta frecuencia.
 */
void Timer0IntHandler(void) {
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	ulHighFrequencyTimerTicks++;
}

/**
 * @brief Obtiene el número de ticks del temporizador de alta frecuencia.
 * 
 * @return unsigned long Número de ticks.
 */
unsigned long getTimerTicks(void) {
  	return ulHighFrequencyTimerTicks;
}

/**
 * @brief Tarea para obtener estadísticas de tareas.
 * 
 * Esta tarea obtiene estadísticas del uso de la CPU por las tareas
 * y envía los datos a través de UART.
 * 
 * @param pvParameters Parámetro de la tarea (no utilizado).
 */
static void vTopTask(void *pvParameters) {
	TaskStatus_t *pxTaskStatusArray;

	// obtiene el número total de tareas actualmente activas en el sistema
	UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();

	// se solicita memoria suficiente para almacenar un array de estructuras
	pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

	while (true) {
		volatile UBaseType_t uxArraySize, x;
		unsigned long ulTotalRunTime, ulStatsAsPercentage;

		if (pxTaskStatusArray != NULL) {
			// obtiene la cant de tareas para las cuales hay información sobre su estado.
			uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );

			/* tiempo total de ejecución de todas las tareas desde el inicio del sistema 
			dividido 100 para calculo de porcentaje*/
			ulTotalRunTime /= 100UL;

			// Enviar via UART
			sendUART0("\r");

			if (ulTotalRunTime > 0) {// para evitar divisiones por cero
				char* current_N[2] = {};
				sendUART0("\r\n\r\n----------- ESTADISTICAS DE TAREAS  -----------\r\n\r\n");
				sendUART0("Filter N: ");
				sendUART0(itoa(getN(), current_N, 10)); // Actual valor de N
				sendUART0("\r\n\r\n");
				sendUART0("Tarea\tTiempo\tCPU%\tStack Libre\r\n");

				// recorre cada una de las tareas
				for (x = 0; x < uxArraySize; x++) {
					// que porcetaje del tiempo total uso cada tarea.
					ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / ulTotalRunTime;

					char buffer[8];

					// se envian los nombres de cada tarea
					sendUART0(pxTaskStatusArray[x].pcTaskName);
					sendUART0("\t");

					// se envian los tiempos(en ticks) de cada tarea
					itoa(pxTaskStatusArray[x].ulRunTimeCounter, buffer, 10);
					sendUART0(buffer);
					sendUART0("\t");

					// Se envia el uso de CPU% de cada tarea 
					if (ulStatsAsPercentage > 0UL) {
						itoa(ulStatsAsPercentage, buffer, 10);
						sendUART0(buffer);
						sendUART0("%\t");
					} else {
						sendUART0("<1%\t");
					}

					// Envia el Stack Free de la pila
					// Más cerca de 0, más cerca del overflow
					itoa(uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle), buffer, 10);
					sendUART0(buffer);
					sendUART0(" words\r\n");
				}
			}
		}

		vTaskDelay(mainTOP_DELAY); // 5 segundos
	}
}

/**
 * @brief Tarea para monitorear los valores mínimos de stack.
 * 
 * Esta tarea monitorea los valores mínimos de stack de las tareas activas.
 * 
 * @param pvParameters Parámetro de la tarea (no utilizado).
 */
static void vMonitorTask(void *pvParameters) {
    char buffer[50];
    UBaseType_t uxHighWaterMarkSensor, uxHighWaterMarkReceiver, uxHighWaterMarkDisplay;

    for (;;) {
		// Obtiene el high water mark de cada tarea
        uxHighWaterMarkSensor = uxTaskGetStackHighWaterMark(xHandleSensorTask);
        uxHighWaterMarkReceiver = uxTaskGetStackHighWaterMark(xHandleReceiverTask);
        uxHighWaterMarkDisplay = uxTaskGetStackHighWaterMark(xHandleDisplayTask);

        // Envia uso de la pila de la tarea del sensor
        itoa(uxHighWaterMarkSensor, buffer, 10);
        sendUART0("Sensor Task Stack High Water Mark: ");
        sendUART0(buffer);
        sendUART0("\n");

        // Envia uso de la pila de la tarea del receptor
        itoa(uxHighWaterMarkReceiver, buffer, 10);
        sendUART0("Receiver Task Stack High Water Mark: ");
        sendUART0(buffer);
        sendUART0("\n");

        // Envia uso de la pila de la tarea de display
        itoa(uxHighWaterMarkDisplay, buffer, 10);
        sendUART0("Display Task Stack High Water Mark: ");
        sendUART0(buffer);
        sendUART0("\n");
		sendUART0("--------------------------------------");
		sendUART0("\n");
        
        vTaskDelay(mainMONITOR_DELAY); // Verifica cada 10 segundos
    }
}