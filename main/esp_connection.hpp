#ifndef ESP_CONNECTION_H
#define ESP_CONNECTION_H

#include "octopus-utils/octopus-connection/connection.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"

class ESPConnection : public OctopusConnection {
public: 
    ESPConnection(std::string uri);

protected:
    // Not useful ? Or to replace with a kind of mutex mechanism to wait for close
    virtual void join() override {};

    virtual bool _connect() override;
    virtual bool _sendText(const char* data, int len) override;
    virtual void _close() override;

    static void eventHandler(void* connection_ptr, esp_event_base_t base, int32_t event_id, void* event_data);

    std::shared_ptr<esp_websocket_client_handle_t> client;
};

#ifdef __INTELLISENSE__
    #pragma diag_suppress 2925
#endif

#endif