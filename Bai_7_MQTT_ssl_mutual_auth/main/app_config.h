#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// Khai bao mot enum cac trang thai config mong muon
typedef enum
{
    PROVISION_ACCESSPOINT = 0,
    PROVISION_SMARTCONFIG
} provision_type_t;

// Khai bao cac nguyen mau ham:
void app_config(void);
#endif
