#include "DriverLib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define ValTempQUEUE_SIZE    (10) // val cola de sensores
#define mainCHECK_TASK_PRIORITY    (tskIDLE_PRIORITY + 3)
#define mainCHECK_DELAY    ( ( TickType_t ) 100 / portTICK_PERIOD_MS ) // 0.1 segundos -> Frecuencia 10Hz (1/10)
#define N 5 // Número de mediciones para el filtro pasa bajos

// Prototipos de las tareas
static void vTemperatureSensorTask(void *pvParameters);
static void vReceiverTask(void *pvParameters);

int getN(void);
void setN(int new_N);
void updateArray(int[], int, int);

// Definición de la cola que contiene los valores de temperatura
QueueHandle_t xValTempQueue;
QueueHandle_t xValFilterQueue;

static int N_filter;

#define N_ARRAY    20
#define MAX_TEMP   40
#define MIN_TEMP   1

int main(void)
{
    setN(15);
    // Inicializar la cola
    xValTempQueue = xQueueCreate(ValTempQUEUE_SIZE, sizeof(int));
    xValFilterQueue = xQueueCreate(ValTempQUEUE_SIZE, sizeof(int));

    // Crear las tareas
    xTaskCreate(vTemperatureSensorTask, "TemperatureSensor", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY - 2, NULL);
    xTaskCreate(vReceiverTask, "Filter", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY - 2, NULL);

    // Iniciar el scheduler de FreeRTOS
    vTaskStartScheduler();

    // El código no debería llegar aquí
    return 0;
}

static void vTemperatureSensorTask(void *pvParameters) {
    const TickType_t xDelaySensor = mainCHECK_DELAY;
    int temperature_value = 15;
    while (true) {
        /* Sensor gets temperature value */
        if (temperature_value <= MAX_TEMP) {
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
