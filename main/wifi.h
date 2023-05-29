#ifndef MAIN_WIFI_H
#define MAIN_WIFI_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define WIFI_OK 0
#define WIFI_NOT_OK 1

static EventGroupHandle_t s_wifi_event_group;

void connect_wifi(const char* ssid, const char* password);
int wait_wifi_connection();

#endif