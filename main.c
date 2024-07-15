#include "DriverLib.h"

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


#include "queue.h"
#include "semphr.h"

#include "integer.h"
#include "PollQ.h"
#include "semtest.h"
#include "BlockQ.h"


// Declaración de la función de la tarea del sensor de temperatura
//void vTemperatureSensorTask(void *pvParameters);

int main(void)
{
    // // Inicializar el generador de números aleatorios
    // srand(time(NULL));

    // // Crear la tarea del sensor de temperatura
    // xTaskCreate(vTemperatureSensorTask, "Temperature Sensor", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // // Iniciar el scheduler de FreeRTOS
    vTaskStartScheduler();

    // El código no debería llegar aquí
    //for (;;);

    return 0;
}
void vUART_ISR(void){
}
void setupTimer0(void) {
}
unsigned long getTimerTicks(void) {
  	//return ulHighFrequencyTimerTicks;
}
// void vTemperatureSensorTask(void *pvParameters)
// {
//     TickType_t xLastWakeTime;
//     const TickType_t xFrequency = pdMS_TO_TICKS(100); // Frecuencia de 10 Hz (100 ms)

//     // Inicializar la variable xLastWakeTime con el tiempo actual
//     xLastWakeTime = xTaskGetTickCount();

//     for (;;)
//     {
//         // Generar un valor aleatorio de temperatura (por ejemplo, entre 20 y 30 grados Celsius)
//         int temperature = rand() % 11 + 20;

//         // Imprimir el valor de temperatura
//         printf("Temperature: %d C\n", temperature);

//         // Esperar hasta el siguiente ciclo
//         vTaskDelayUntil(&xLastWakeTime, xFrequency);
//     }
// }
