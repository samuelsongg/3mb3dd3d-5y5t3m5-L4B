/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/ip4_addr.h"

#include "FreeRTOS.h"
#include "task.h"
#include "ping.h"
#include "message_buffer.h"

#include "hardware/gpio.h"
#include "hardware/adc.h"

#define mbaTASK_MESSAGE_BUFFER_SIZE       ( 60 )

#ifndef PING_ADDR
#define PING_ADDR "142.251.35.196"
#endif
#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define TEST_TASK_PRIORITY				( tskIDLE_PRIORITY + 1UL )

// Define message buffers.
static MessageBufferHandle_t movingAvgReceiveCMB;
static MessageBufferHandle_t simpleAvgReceiveCMB;
static MessageBufferHandle_t onBoardTempSendCMB;
static MessageBufferHandle_t movingAvgSendCMB;
static MessageBufferHandle_t simpleAvgSendCMB;

float read_onboard_temperature() {
    
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    return tempC;
}

// Task to print out all the temperature data.
void print_task(__unused void *params) {    
    float fPrintTempData;
    float fPrintMovingAvgData;
    float fPrintSimpleAvgData;

    while(true) {
        xMessageBufferReceive( 
            onBoardTempSendCMB,                 /* The message buffer to receive from. */
            (void *) &fPrintTempData,           /* Location to store received data. */
            sizeof( fPrintTempData ),           /* Maximum number of bytes to receive. */
            portMAX_DELAY );                    /* Wait indefinitely */

        xMessageBufferReceive( 
            movingAvgSendCMB,                   /* The message buffer to receive from. */
            (void *) &fPrintMovingAvgData,      /* Location to store received data. */
            sizeof( fPrintMovingAvgData ),      /* Maximum number of bytes to receive. */
            portMAX_DELAY );                    /* Wait indefinitely */

        xMessageBufferReceive( 
            simpleAvgSendCMB,                   /* The message buffer to receive from. */
            (void *) &fPrintSimpleAvgData,      /* Location to store received data. */
            sizeof( fPrintSimpleAvgData ),      /* Maximum number of bytes to receive. */
            portMAX_DELAY );                    /* Wait indefinitely */

        printf("Onboard temperature = %.02f C\n", fPrintTempData);
        printf("Moving Average Temperature = %0.2f C\n", fPrintMovingAvgData);
        printf("Simple Average Temperature = %0.2f C\n\n", fPrintSimpleAvgData);
    }
}

// Task to obtain and calculate the simple average temperature.
void simple_avg_task(__unused void *params) {
    float fSimpleAvgReceivedData;
    float sum = 0;
    float result = 0;
    
    static int count = 0;

    while(true) {
        xMessageBufferReceive( 
            simpleAvgReceiveCMB,                   /* The message buffer to receive from. */
            (void *) &fSimpleAvgReceivedData,      /* Location to store received data. */
            sizeof( fSimpleAvgReceivedData ),      /* Maximum number of bytes to receive. */
            portMAX_DELAY );                       /* Wait indefinitely */

        sum += fSimpleAvgReceivedData;
        count++;
        result = sum / count;
        
        xMessageBufferSend(
        simpleAvgSendCMB,                          /* The message buffer to write to. */
            (void *) &result,                      /* The source of the data to send. */
            sizeof( result ),                      /* The length of the data to send. */
            0 );                                   /* Do not block, should the buffer be full. */
    }
}

/* A Task that obtains the data every 1000 ticks from the inbuilt temperature sensor (RP2040) and sends it out. */
void temp_task(__unused void *params) {
    float temperature = 0.0;

    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    while(true) {
        vTaskDelay(1000);
        temperature = read_onboard_temperature();
        
        xMessageBufferSend( 
            movingAvgReceiveCMB,      /* The message buffer to write to. */
            (void *) &temperature,    /* The source of the data to send. */
            sizeof( temperature ),    /* The length of the data to send. */
            0 );                      /* Do not block, should the buffer be full. */

        xMessageBufferSend(
            simpleAvgReceiveCMB,      /* The message buffer to write to. */
            (void *) &temperature,    /* The source of the data to send. */
            sizeof( temperature ),    /* The length of the data to send. */
            0 );                      /* Do not block, should the buffer be full. */

        xMessageBufferSend(
            onBoardTempSendCMB,       /* The message buffer to write to. */
            (void *) &temperature,    /* The source of the data to send. */
            sizeof( temperature ),    /* The length of the data to send. */
            0 );                      /* Do not block, should the buffer be full. */
    }
}

/* A Task that indefinitely waits for data from temp_task via message buffer. Once received, it will calculate the moving average. */
void moving_avg_task(__unused void *params) {
    float fOnBoardTempData;
    float sum = 0;
    float result = 0;
    
    static float data[10] = {0};
    static int index = 0;
    static int count = 0;

    while(true) {
        xMessageBufferReceive( 
            movingAvgReceiveCMB,                 /* The message buffer to receive from. */
            (void *) &fOnBoardTempData,          /* Location to store received data. */
            sizeof( fOnBoardTempData ),          /* Maximum number of bytes to receive. */
            portMAX_DELAY );                     /* Wait indefinitely */

        sum -= data[index];                      // Subtract the oldest element from sum
        data[index] = fOnBoardTempData;          // Assign the new element to the data
        sum += data[index];                      // Add the new element to sum
        index = (index + 1) % 10;                // Update the index - make it circular
        
        if (count < 10) count++;                 // Increment count till it reaches 10

        result = sum / count;

        xMessageBufferSend(
            movingAvgSendCMB,                    /* The message buffer to write to. */
            (void *) &result,                    /* The source of the data to send. */
            sizeof( result ),                    /* The length of the data to send. */
            0 );                                 /* Do not block, should the buffer be full. */
    }
}

void vLaunch( void) {
    // Initialise the tasks.
    TaskHandle_t printTask;
    xTaskCreate(print_task, "PrintThread", configMINIMAL_STACK_SIZE, NULL, TEST_TASK_PRIORITY, &printTask);
    TaskHandle_t simpleAvgTask;
    xTaskCreate(simple_avg_task, "SimpleAvgThread", configMINIMAL_STACK_SIZE, NULL, 7, &simpleAvgTask);
    TaskHandle_t tempTask;
    xTaskCreate(temp_task, "TempThread", configMINIMAL_STACK_SIZE, NULL, 8, &tempTask);
    TaskHandle_t movingAvgTask;
    xTaskCreate(moving_avg_task, "MovingAvgThread", configMINIMAL_STACK_SIZE, NULL, 5, &movingAvgTask);

    // Initialise the message buffers.
    movingAvgReceiveCMB = xMessageBufferCreate(mbaTASK_MESSAGE_BUFFER_SIZE);
    simpleAvgReceiveCMB = xMessageBufferCreate(mbaTASK_MESSAGE_BUFFER_SIZE);  
    onBoardTempSendCMB = xMessageBufferCreate(mbaTASK_MESSAGE_BUFFER_SIZE);  
    movingAvgSendCMB = xMessageBufferCreate(mbaTASK_MESSAGE_BUFFER_SIZE);
    simpleAvgSendCMB = xMessageBufferCreate(mbaTASK_MESSAGE_BUFFER_SIZE);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

int main( void )
{
    stdio_init_all();

    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
#if ( portSUPPORT_SMP == 1 )
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if ( portSUPPORT_SMP == 1 ) && ( configNUM_CORES == 2 )
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif ( RUN_FREERTOS_ON_CORE == 1 )
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}

/*** end of file ***/
