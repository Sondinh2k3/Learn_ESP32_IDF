#ifndef INPUT_IOT_H
#define INPUT_IOT_H

#include "esp_err.h"
#include "hal/gpio_types.h"

typedef void (*input_callback_t)(int);

// Tạo một enum sườn nào: Sườn lên, sườn xuống hay cả 2
typedef enum
{
    LO_TO_HI = 1,
    HI_TO_LO,
    ANY_EDLE
} interrupt_type_edge_t;

void input_io_create(gpio_num_t gpio_num, gpio_int_type_t intr_type);
int input_io_get_level(gpio_num_t gpio_num);
void input_set_callback(void *cb);

#endif