#include "esp_connection.hpp"

#include <functional>
#include <iostream>

// #include "portmacro.h"
// #include "condi"
#include <unistd.h>

ESPConnection::ESPConnection(std::string uri) : OctopusConnection(uri) {
}

static char* curr_payload = nullptr;
static int curr_payload_len = 0;

void ESPConnection::eventHandler(void* connection_ptr, esp_event_base_t base, int32_t event_id, void* event_data) {
    ESPConnection* connection = (ESPConnection*) connection_ptr;

    std::cout << "..." << std::endl;

    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI("[ESPConnection]", "WEBSOCKET_EVENT_CONNECTED");
        {
            bool wasConnected = !connection->_listenMutex.try_lock();

            if(wasConnected)
                std::cerr << "[ESPConnection] Already connected ?? Should not be!" << std::endl;

            connection->_connecting.store(false);

            if(connection->_onConnectionStatus != nullptr)
                connection->_onConnectionStatus(wasConnected, true);
        }

        break;
    case WEBSOCKET_EVENT_DATA:
        std::cout << (int) data->op_code << std::endl;

        // Ping frame
        if(data->op_code == WS_TRANSPORT_OPCODES_PING) {
            const char* data = "\0";
            esp_websocket_client_send_with_opcode(*connection->client, WS_TRANSPORT_OPCODES_PONG, (const uint8_t*) data, 1, portMAX_DELAY);
            return;
        }

        // std::cout << data->data_len << std::endl;

        if(data->data_len <= 0)
            break;
        
        if (data->payload_offset == 0) {
            if(curr_payload) {
                free(curr_payload);
                curr_payload = nullptr;
            }

            curr_payload = (char*) malloc(data->payload_len * sizeof(char));
            curr_payload_len = data->payload_len;

            memcpy(curr_payload, data->data_ptr, data->data_len);
        } else {
            if (curr_payload == nullptr) {
                std::cout << "Got data with offset while curr_payload is nullptr; Aborting" << std::endl;
                break;
            }

            if (data->payload_len != curr_payload_len) {
                std::cout << "Got data with offset with payload_len != curr_payload_len (" << data->payload_len  << "!=" << curr_payload_len << "); Aborting" << std::endl;
                break;
            }

            memcpy(curr_payload + data->payload_offset, data->data_ptr, data->data_len); 
        }

        std::cout << data->fin << std::endl;

        if (data->payload_offset + data->data_len == data->payload_len) {
            connection->onText(curr_payload, curr_payload_len);
            free(curr_payload);
            curr_payload = nullptr;
            curr_payload_len = 0;
        }

        // std::cout << data->payload_len << std::endl;
        // std::cout << data->data_len << std::endl;
        // std::cout << data->payload_offset << std::endl;
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI("[ESPConnection]", "WEBSOCKET_EVENT_DISCONNECTED");
        goto websocket_disconnect;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI("[ESPConnection]", "WEBSOCKET_EVENT_ERROR");
websocket_disconnect:
        {
            bool wasConnected = connection->isConnected();
            if (wasConnected) {
                connection->_listenMutex.unlock();
            } else {
                connection->_connecting.store(false);
            }

            if(connection->_onConnectionStatus != nullptr)
                connection->_onConnectionStatus(wasConnected, false);
        }
        break;
    }
}

bool ESPConnection::_connect() {
    esp_websocket_client_config_t websocket_cfg = {};
    websocket_cfg.uri = _uri.c_str();
    websocket_cfg.keep_alive_enable = true;
    websocket_cfg.task_stack = 10 * 1024;
    websocket_cfg.disable_auto_reconnect = true;
    websocket_cfg.reconnect_timeout_ms = 1;

    client = std::make_shared<esp_websocket_client_handle_t>(esp_websocket_client_init(&websocket_cfg));

    esp_websocket_register_events(*client, WEBSOCKET_EVENT_ANY, ESPConnection::eventHandler, this);

    esp_websocket_client_start(*client);

    _connecting.store(true);

    return true;
}

bool ESPConnection::_sendText(const char* data, int len) {
    if (esp_websocket_client_is_connected(*client)) {
        esp_websocket_client_send_text(*client, data, len, portMAX_DELAY);
        return true;
    }

    return false;
}

void ESPConnection::_close() {
    if(this->isConnected())
        esp_websocket_client_close(*this->client, portMAX_DELAY);


    // EventBits_t bits = xEventGroupWaitBits((*this->client)->status_bits,
    //         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
    //         pdFALSE,
    //         pdFALSE,
    //         portMAX_DELAY);

    std::cout << "trying destroy" << std::endl;

    // esp_websocket_
    usleep(5000);
    esp_websocket_client_destroy(*this->client);

    std::cout << "websocket client end" << std::endl;
}