#include "DriverLib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
//#includeain <time.h>

#define ValTEmpQUEUE_SIZE	( 10 ) // val cola de sensores}
#define mainCHECK_TASK_PRIORITY	( tskIDLE_PRIORITY + 3 )
#define mainCHECK_DELAY	( ( TickType_t ) 100 / portTICK_PERIOD_MS ) // 0.1 segundos -> Frecuencia 10Hz (1/10)

// Prototipos de las tareas
static void vTemperatureSensorTask(void *pvParameters);
//void vReceiverTask(void *pvParameters);

// Definición de la cola que contiene los valores de temperatura
QueueHandle_t xValTempQueue;

#define MAX_TEMP   40
#define MIN_TEMP   1

int main(void)
{

    // // Inicializar la cola
    xValTempQueue = xQueueCreate(ValTEmpQUEUE_SIZE, sizeof(int));

    // Crear las tareas
    xTaskCreate(vTemperatureSensorTask, "TemperatureSensor", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY - 2, NULL);
    //xTaskCreate(vReceiverTask, "Receiver", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // Iniciar el scheduler de FreeRTOS
    vTaskStartScheduler();

    // El código no debería llegar aquí
    //for (;;);

    return 0;
}

static void vTemperatureSensorTask(void *pvParameters) {
	const TickType_t xDelaySensor = mainCHECK_DELAY;
	int temperature_value = 20;
	while (true) {
		/* Sensor gets temperature value */
		if(temperature_value <= MAX_TEMP) {
			temperature_value++;
		} else {
			temperature_value = MIN_TEMP;
		}
        
		xQueueSend(xValTempQueue, &temperature_value, portMAX_DELAY);

		vTaskDelay(xDelaySensor); // waits mainCHECK_DELAY milliseconds for 10Hz frequency
	}
}


// void vTemperatureSensorTask(void *pvParameters) {
//     float temperature;
//     srand((unsigned int)time(NULL));
//     const TickType_t xDelay = pdMS_TO_TICKS(100); // 10 Hz -> 100 ms

//     for (;;) {
//         // Generar un valor aleatorio de temperatura entre 0 y 40
//         temperature = (float)(rand() % 41);
//         // Enviar el valor a la cola
//         if (xQueueSend(xQueue, &temperature, portMAX_DELAY) != pdPASS) {
//             printf("Failed to send to queue.\n");
//         }
//         // Esperar 100 ms
//         vTaskDelay(xDelay);
//     }
// }

void vReceiverTask(void *pvParameters) {
    // float receivedTemperature;

    // for (;;) {
    //     // Recibir el valor de la cola
    //     if (xQueueReceive(xQueue, &receivedTemperature, portMAX_DELAY) == pdPASS) {
    //         // Mostrar el valor recibido
    //         printf("Received Temperature: %.2f °C\n", receivedTemperature);
    //     }
    // }
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
