/*
 * Bài tập: Bấm nút nhấn và toggle LED
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "input_iot.h"
#include "output_iot.h"

// Định nghĩa các chân: Nút nhấn và LED
#define BUTTON GPIO_NUM_12
#define LED GPIO_NUM_4

void Button_handle(int pin)
{
    if (pin == BUTTON)
    {
        output_io_toggle(LED);
    }
}

void app_main(void)
{
    input_io_create(BUTTON, HI_TO_LO);
    output_io_create(LED);
    input_set_callback(Button_handle);
}
