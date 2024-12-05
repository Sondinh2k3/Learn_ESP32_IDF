#include <stdio.h>
#include <esp_log.h>
#include "output_iot.h"
#include <driver/gpio.h>
#include "rom/gpio.h"
#include "esp_attr.h"

// Hàm khởi tạo chân Input
void output_io_create(gpio_num_t gpio_num)
{
    // Cấu hình số chân GPIO
    esp_rom_gpio_pad_select_gpio(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT);
}

// Hàm lấy giá trị logic của chân Input
void output_io_set_level(gpio_num_t gpio_num, int level)
{
    gpio_set_level(gpio_num, level);
}

void output_io_toggle(gpio_num_t gpio_num)
{
    int old_level = gpio_get_level(gpio_num);
    gpio_set_level(gpio_num, 1 - old_level);
}
