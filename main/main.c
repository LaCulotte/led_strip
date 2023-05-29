#include <stdio.h>

#include "wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "nvs_flash.h"

#include "esp_log.h"

#define PIN_ERR GPIO_NUM_33

#define TAG "main"

#define WIFI_SSID      "SFR-2598"
#define WIFI_PASS      "D4YLE4NQGDU4"
// #define WIFI_SSID      "Giga Wifi"
// #define WIFI_PASS      "aaaaaaaa"

extern void start_app(void);

void app_main(void)
{
    gpio_reset_pin(PIN_ERR);

    gpio_set_direction(PIN_ERR, GPIO_MODE_OUTPUT);

    ESP_ERROR_CHECK(gpio_set_level(PIN_ERR, 1));

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    connect_wifi(WIFI_SSID, WIFI_PASS);
    int wifi_res = wait_wifi_connection();
    if (wifi_res != WIFI_OK) {
        ESP_LOGI(TAG, "Wifi connection to %s failed", WIFI_SSID);
        return;
    }

    start_app();
}