#include <stdio.h>
#include <esp_log.h>
#include "input_iot.h"
#include <driver/gpio.h>
#include "rom/gpio.h"
#include "esp_attr.h"

// Cần có bao nhiêu hàm Input?
// Thư viện này sẽ chứa các hàm Input gì?

input_callback_t input_callback = NULL;
static void gpio_input_handler(void *arg);
// Hàm ngắt cho chân GPIO

static void IRAM_ATTR gpio_input_handler(void *arg)
{
    int gpio_num = (uint32_t)arg;
    input_callback(gpio_num);
}

// Hàm khởi tạo chân Input
void input_io_create(gpio_num_t gpio_num, gpio_int_type_t type)
{
    // Cấu hình số chân GPIO
    esp_rom_gpio_pad_select_gpio(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(gpio_num, type);
    gpio_install_isr_service(0);                                          // Dịch vụ ngắt GPIO
    gpio_isr_handler_add(gpio_num, gpio_input_handler, (void *)gpio_num); // Gán hàm ngắt
}

// Hàm lấy giá trị logic của chân Input
int input_io_get_level(gpio_num_t gpio_num)
{
    return gpio_get_level(gpio_num);
}

// Hàm set callback ra ngoài để xử lý
void input_set_callback(void *cb)
{
    input_callback = cb;
}