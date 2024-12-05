/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "output_iot.h"
#include "driver/gpio.h"
#include "input_iot.h"

#define NUM_TIMERS 2
/* An array to hold handles to the created timers. */
TimerHandle_t xTimers[NUM_TIMERS];
EventGroupHandle_t xEventGroup;

// Mối một even là một bit
#define BIT_EVENT_BUTTON_PRESS (1 << 0)
#define BIT_EVENT_UART_RECV (1 << 1)

/* Task to be created. */
void vTask1(void *pvParameters)
{

    for (;;)
    {
        /* Wait a maximum of 100ms for either bit 0 or bit 4 to be set within
       the event group. Clear the bits before exiting. */
        EventBits_t uxBits = xEventGroupWaitBits(
            xEventGroup,                                  /* The event group being tested. */
            BIT_EVENT_BUTTON_PRESS | BIT_EVENT_UART_RECV, /* The bits within the event group to wait for. */
            pdTRUE,                                       /* BIT_0 & BIT_1 should be cleared before returning. */
            pdFALSE,                                      /* Don't wait for both bits, either bit will do. */
            portMAX_DELAY);                               /* Wait a maximum of 100ms for either bit to be set. */

        if (uxBits & BIT_EVENT_BUTTON_PRESS)
        {
            printf("BUTTON PRESS\n");
            output_io_toggle(4);
        }
        else if (uxBits & BIT_EVENT_UART_RECV)
        {
            printf("UART DATA\n");
            //...
        }
    }
}

void vTimerCallback(TimerHandle_t xTimer)
{
    /* Optionally do something if the pxTimer parameter is NULL. */
    configASSERT(xTimer);

    /* The number of times this timer has expired is saved as the
       timer's ID. Obtain the count. */
    int ulCount = (uint32_t)pvTimerGetTimerID(xTimer);

    /* Increment the count, then test to see if the timer has expired
       ulMaxExpiryCountBeforeStopping yet. */
    if (ulCount == 0)
    {
        output_io_toggle(4);
    }
    else if (ulCount == 1)
    {
        printf("Hello\n");
    }
}

void button_callback(int pin)
{
    if (pin == GPIO_NUM_0)
    {
        BaseType_t pxHigherPriorityTaskWoken;
        xEventGroupSetBitsFromISR(xEventGroup, BIT_EVENT_BUTTON_PRESS, &pxHigherPriorityTaskWoken);
    }
}

void app_main(void)
{

    /* Create then start some timers. Starting the timers before
        the RTOS scheduler has been started means the timers will start
        running immediately that the RTOS scheduler starts. */
    xTimers[0] = xTimerCreate(/* Just a text name, not used by the RTOS kernel. */
                              "TimerBlink",
                              /* The timer period in ticks, must be greater than 0. */
                              pdMS_TO_TICKS(500),
                              /* The timers will auto-reload themselves when they expire. */
                              pdTRUE,
                              /* The ID is used to store a count of the number of times the
                                 timer has expired, which is initialised to 0. */
                              (void *)0,
                              /* Each timer calls the same callback when it expires. */
                              vTimerCallback);

    xTimers[1] = xTimerCreate(/* Just a text name, not used by the RTOS kernel. */
                              "TimerPrint",
                              /* The timer period in ticks, must be greater than 0. */
                              pdMS_TO_TICKS(1000),
                              /* The timers will auto-reload themselves when they expire. */
                              pdTRUE,
                              /* The ID is used to store a count of the number of times the
                                 timer has expired, which is initialised to 0. */
                              (void *)1,
                              /* Each timer calls the same callback when it expires. */
                              vTimerCallback);

    output_io_create(4);
    input_io_create(0, HI_TO_LO);
    input_set_callback(button_callback);

    // xTimerStart(xTimers[0], 0);
    // xTimerStart(xTimers[1], 0);

    xEventGroup = xEventGroupCreate();

    /* Create the task, storing the handle. */
    xTaskCreate(
        vTask1,   /* Function that implements the task. */
        "Task 1", /* Text name for the task. */
        1024 * 2, /* Stack size in words, not bytes. */
        NULL,     /* Parameter passed into the task. */
        4,        /* Priority at which the task is created. */
        NULL);    /* Used to pass out the created task's handle. */
}
