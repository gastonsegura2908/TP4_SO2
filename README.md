# TP4 - SISTEMAS OPERATIVOS 2
- Gaston Marcelo Segura
## CONCEPTOS CLAVE

### QEMU
QEMU es un emulador y virtualizador de código abierto que permite emular diversos sistemas y arquitecturas de hardware.

### Stellaris LM3S811
El Stellaris LM3S811 es un microcontrolador de Texas Instruments que pertenece a la familia Stellaris, basada en la arquitectura ARM Cortex-M3. Las características principales de este microcontrolador incluyen:
- Procesador ARM Cortex-M3
- Memoria Flash y SRAM integradas
- Periféricos como temporizadores, ADC, UART, SPI, I2C, GPIO, etc.
- Soporte para aplicaciones de tiempo real

### FreeRTOS
FreeRTOS es un sistema operativo de tiempo real (RTOS) para microcontroladores. Permite gestionar tareas concurrentes, temporizadores, colas de mensajes, y más, facilitando el desarrollo de aplicaciones embebidas.
- Se utiliza el archivo de configuración FreeRTOSConfig.h para definir las características específicas del sistema, como el tamaño de pila de las tareas, la prioridad máxima, y otros parámetros.

### OBJETIVO
Desarrollar y probar una aplicación basada en FreeRTOS para el microcontrolador Stellaris LM3S811, utilizando QEMU para la emulación.


## DESARROLLO 
En **primer** lugar se genera una tarea, llamada **`vTemperatureSensorTask`** , que simula un sensor de temperatura generando valores con una frecuencia de 10 Hz( 0.1 segundos). La temperatura minima es 10 grados y la maxima es 30 grados, entonces en el ciclo while se va generando la funcion, similar a los que ocurriria con un sensor obteniendo la temperatura ambiente. Luego de esto, el valor de temperatura es enviado a la cola llamada xValTempQueue para su posterior uso. Finalmente se produce el delay de 0.1 segundos(simulando la frecuencia de actualización del sensor) a traves de la funcion vTaskDelay(mainSENSOR_DELAY) la cual es una función de FreeRTOS que suspende la ejecución de la tarea durante el tiempo especificado en el parámetro; este valor se obtiene desde la linea `( ( TickType_t ) 100 / portTICK_PERIOD_MS )`:  TickType_t realiza el casteo, 100  representa el valor del retardo en milisegundos (0,1 seg equivalente a 100 ms), y se divide por el periodo de un tick del sistema (portTICK_PERIOD_MS),esto convierte el retardo de milisegundos a unidades de ticks.
```
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

        vTaskDelay(xDelaySensor); 
    }
}
```
---
En **segundo lugar**, se realiza una tarea, denominada vReceiverTask, que recibe los valores del sensor y aplica un filtro pasa bajos, donde cada valor resultante es el promedio de las ultimas N mediciones.  
Se observa que primero se lee el valor de temperatura desde la cola `xValTempQueue`(la cual proviene de la tarea `vTemperatureSensorTask`), se añade a un array y se calcula el promedio entre los valores del array, de las ultimas N mediciones. El valor resultante se envia a una cola, llamada `xValFilterQueue`, la cual sera utilizada por la tarea del display .
```
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
```

Para esta tarea, y su posterior uso en otra, se realizan ademas un setter y un getter para el valor de N:
```
void setN(int new_N) {
    N_filter = new_N;
}

int getN(void) {
    return N_filter;
}

```
---

En **tercer** lugar se realiza una tarea, denominada  ` vDisplayTask`, que grafica en el display los valores de temperatura en el tiempo.

![image](https://github.com/user-attachments/assets/36dccd44-19a7-4b8e-b6ba-ce1f221b8667)

Se crea un array del tamaño de la cantidad de columnas de display (96), y se va llenando a medida que va leyendo los valores que obtiene desde la cola que proviene de la tarea del filtrado; por lo tanto va actualizando ese array a medida que van llegando los valores y va mostrando en pantalla tanto el numero N, como el valor filtrado actual, y el grafico del valor en el tiempo, todo esto ayudandose con la funcion `bitMapping` . Esta funcion mapea los valores filtrados a un patron de bits, siguiendo el hecho de que el display esta compuesto por una pantalla de 96 columnas x 16 filas, las cuales estan dividas en dos, formando una pantalla superior(8 filas) y una pantalla inferior(8 filas); esas filas son 1 byte(8 bits) en vertical, el cual es el valor que devuelve la funcion bitMapping.

```
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
```

```
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

```

---

En **cuarto** lugar se realiza la funcion de poder cambiar el N del filtro, a traves de recibir comandos por la interfaz UART. En este caso, al ingresar "+" aumenta el N, y al ingresar "-" disminuye, en ambos casos tienen un limite ,el cual es N=1 como minimo, y N=20 como maximo.
Para el funcionamiento de UART en primer lugar se debe realizar la configuracion del hardware:

```
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
```
Y luego finalmente, las acciones se realizan en el handler de la interrupcion por UART, utilizando el setter y getter de N, y la funcion sendUART0 para notificar lo sucedido:
```
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
```

---
En **quinto** lugar, para calcular el stack necesario para cada tarea, se utiliza la funcion ` uxTaskGetStackHighWaterMark` , la cual es una función que en FreeRTOS  se utiliza para obtener el **"high water mark"** de la pila de una tarea, es decir, el nivel más bajo de la pila (en términos de espacio libre) que ha alcanzado desde que la tarea comenzó a ejecutarse. Con esto se puede ajustar el tamaño de la pila de la tarea para evitar desbordamientos sin desperdiciar memoria. La función devuelve el número de palabras (no bytes) que permanecieron sin usar en la pila en su punto más bajo. Para esto, primero se crea una tarea adicional(denominada `vMonitorTask`) que monitoree periódicamente el uso de la pila de las otras tareas:

```
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
```
Previo a esto se han creado y asignado handlers para cada una de las tareas:
```
    xTaskCreate(vTemperatureSensorTask, "TemperatureSensor", configSENSOR_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY,  &xHandleSensorTask);
    xTaskCreate(vReceiverTask, "Filter", configFILTER_STACK_SIZE, NULL, RECEIVER_TASK_PRIORITY, &xHandleReceiverTask);
    xTaskCreate(vDisplayTask, "Display", configDISPLAY_STACK_SIZE, NULL, DISPLAY_TASK_PRIORITY, &xHandleDisplayTask);
```
En FreeRTOS, el último parámetro de la función xTaskCreate es un puntero a un TaskHandle_t que, si no es NULL, se utilizará para almacenar el manejador (handle) de la tarea recién creada. Este manejador permite interactuar con la tarea posteriormente, por ejemplo permitiendo obtener el uso de la pila(uxTaskGetStackHighWaterMark) que es justamente lo que se necesita.
Para obtener los valores de stack necesario para cada task, hay que ir jugando con los valores que nos devuelve  los handlers,partiendo de un valor de stack mas alto de lo necesario, por ej estos:
```
#define configSENSOR_STACK_SIZE                   ((unsigned short) (50))
#define configFILTER_STACK_SIZE                   ((unsigned short) (80)) 
#define configDISPLAY_STACK_SIZE                  ((unsigned short) (245)) 
```
y vemos que los valores devueltos son:

![image](https://github.com/user-attachments/assets/2e3ea63d-f0e3-4768-8493-ff8d7203a406)

por lo tanto se pueden reducir aun mas los valores de la pila, pero siempre tratando de que la High Water Mark no llegue a cero y dejando un margen(Si los valores de High Water Mark se acercan a cero, es indicativo de que la pila es insuficiente por lo tanto se debe aumentar el valor). 
Finalmente se llegan a estos valores de stack:
```
#define configSENSOR_STACK_SIZE                   ((unsigned short) (40))
#define configFILTER_STACK_SIZE                   ((unsigned short) (65)) 
#define configDISPLAY_STACK_SIZE                  ((unsigned short) (150)) 
```
ya que devolvieron los siguientes valores:

![image](https://github.com/user-attachments/assets/276a72dd-d123-41b9-9387-dc751b2bb5ae)

---

En sexto lugar se implementa una tarea tipo top de linux, denominada `vTopTask`, la cual muestra periodicamente estadisticas de las tareas. 
Primero se obtiene cual es la cantidad de tareas a partir de las cuales se puede conseguir informacion de su estado, y luego se va recorriendo una por una. La informacion que se puede conseguir son: que porcentaje del tiempo total uso cada tarea, los tiempos(en ticks) de cada tarea, el uso de CPU% de cada tarea y el Stack Free de la pila; toda esta informacion se envia utilizando la funcion `sendUART0`.

```
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
```
Aqui podemos observar los resultados:

![image](https://github.com/user-attachments/assets/c3b50965-982a-4330-b78b-d34c9f1cac11)

En FreeRTOS, la tarea IDLE es una tarea interna que el sistema operativo crea automáticamente y se ejecuta cuando no hay otras tareas listas para ejecutarse. Su propósito principal es mantener la CPU ocupada cuando no hay trabajo pendiente para otras tareas del sistema, un CPU% alto para la tarea IDLE (como el 86% que se muestra) sugiere que la aplicación tiene mucho tiempo de inactividad, lo cual es generalmente bueno en sistemas embebidos ya que significa que hay capacidad de procesamiento disponible para manejar nuevas tareas o interrupciones.

---

## MAS DETALLES 

_**COLAS**_

xQueueSend en FreeRTOS se utiliza para enviar datos a una cola. Si la cola está llena, la tarea que está enviando los datos se bloqueará hasta que haya espacio disponible en la cola (cuando se usa portMAX_DELAY como el tiempo de espera(representa el máximo tiempo de espera posible para una operación que puede bloquearse, osea la tarea se bloqueará indefinidamente hasta que la operación se complete).
En este caso, la cola xValTempQueue tiene capacidad para almacenar ValTempQUEUE_SIZE (20) elementos de tipo int. Si la cola está llena, xQueueSend bloqueará la tarea vTemperatureSensorTask hasta que otro valor sea extraído de la cola por la tarea vReceiverTask, al utilizar la funcion xQueueReceive().

---

_**PRIORIDADES**_

Las prioridades de las tareas en FreeRTOS determinan el orden en que las tareas son elegidas para ejecutarse. Una tarea con mayor prioridad será ejecutada antes que una tarea con menor prioridad si ambas están listas para ejecutarse.
Para definir las prioridades el punto de referencia es la tarea inactiva (tskIDLE_PRIORITY), la cual tiene la prioridad más baja posible en FreeRTOS, por lo tanto definimos "x" niveles de prioridad mayores a esta tarea:
- vTemperatureSensorTask: Prioridad baja((tskIDLE_PRIORITY + 1). Esta tarea tiene una frecuencia alta (10 Hz) y necesita una prioridad relativamente baja ya que solo se encarga de generar y enviar datos a una cola.
- vReceiverTask: Prioridad media(tskIDLE_PRIORITY + 2).  Esta tarea es más crítica que la tarea del sensor ya que realiza procesamiento de datos que serán utilizados por otras tareas. Tiene una prioridad un nivel por encima del sensor, lo cual es razonable para garantizar que el filtrado de datos ocurra con prioridad sobre la generación de datos.
- vDisplayTask: Prioridad alta(tskIDLE_PRIORITY + 3). Esta tarea no es tan crítica como el procesamiento de datos, pero necesita cierta prioridad para que los datos se muestren en tiempo real.
-  vTopTask: Prioridad baja((tskIDLE_PRIORITY + 1). Esta tarea es menos crítica que las anteriores y puede ejecutarse con una prioridad menor, esto con el objetivo de evitar que interfiera con las tareas principales.

En FreeRTOS, cuando dos tareas tienen la misma prioridad, el planificador utiliza una técnica llamada "round-robin" para repartir el tiempo de la CPU entre esas tareas. Esto significa que el tiempo de la CPU se dividirá de manera equitativa entre todas las tareas que tengan la misma prioridad y estén listas para ejecutarse.

---

_**UART**_

- En la funcion prvSetupHardware nos aseguramos que el microcontrolador esté configurado para usar la UART. Esto generalmente implica configurar los registros de control de UART para establecer la velocidad en baudios, el número de bits de datos, la paridad y los bits de parada.
- mainBAUD_RATE define la velocidad de transmisión de datos en la comunicación serie (baud rate). En este caso, está configurada a 19200 bits por segundo (bps). 19200 bps es una velocidad de comunicación comúnmente utilizada en sistemas embebidos porque proporciona un buen equilibrio entre la velocidad de transmisión y la fiabilidad de los datos. Este valor no es ni demasiado rápido para causar errores en la transmisión ni demasiado lento para afectar el rendimiento del sistema.
- UART nos sirve tanto para enviar datos(utilizando la funcion sendUART0,  donde se transmite bit a bit), como para recibirlos(utilizando el handler de UART, segun si ingreso "+" o "-")

---

_**DISPLAY**_

El display de 16x96 píxeles se divide en dos mitades: la parte superior (8x96) y la parte inferior (8x96). 

![image](https://github.com/user-attachments/assets/174e0874-a296-4296-87e6-d4aeadb59f98)

La función OSRAMStringDraw por dentro utiliza la La función OSRAMWriteArray(g_pucFont[*pcStr - ' '], 5) la cual escribe los datos del carácter en 5 columnas + 1 columna de padding  en caso de que se haya ingresado mas de un caracter, por lo tanto si a la funcion OSRAMStringDraw le mandamos dos carcateres, estos ocuparan 11 columnas(del 0 al 10), con lo cual hay que arrancar en la columna 11 los graficos para no chocar con los primeros numeros.

![image](https://github.com/user-attachments/assets/d87ac9e5-6959-4a72-9e88-a74af7fb9154)

---

_**TIMER**_

-  El Timer0 se usa para generar ticks, que son interrupciones periódicas que el sistema operativo de tiempo real (RTOS) utiliza para controlar el tiempo y la ejecución de tareas. 
- timerINTERRUPT_FREQUENCY (20000UL): timerINTERRUPT_FREQUENCY define la frecuencia a la que se generan las interrupciones del temporizador. En este caso, está configurada a 20,000 interrupciones por segundo (20 kHz).  Este valor asegura que las tareas sensibles al tiempo puedan ejecutarse con alta precisión y regularidad.
- En FreeRTOS, un "tick" es una unidad de tiempo medida por el sistema operativo. FreeRTOS usa un temporizador (Timer) para generar interrupciones periódicas, y cada interrupción se denomina "tick". Este tick es la base del sistema de temporización en FreeRTOS y se utiliza para la gestión de tareas, temporizadores de software y funciones de retardo. El período del tick se configura en el archivo de configuración de FreeRTOS (FreeRTOSConfig.h) mediante la macro configTICK_RATE_HZ. Esta macro define cuántos ticks ocurren por segundo. Por ejemplo, si configTICK_RATE_HZ está definido como 1000( como lo es en este caso), entonces habrá 1000 ticks por segundo, es decir, cada tick ocurre cada 1 ms.



