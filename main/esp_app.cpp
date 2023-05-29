#include "esp_app.hpp"

#include <iostream>
#include <octopus-app/uuid.hpp>
#include <unistd.h>

#include "driver/gpio.h"
// #include "wifi.h"

#include <chrono>


#define PIN_BLUE GPIO_NUM_27
#define PIN_RED GPIO_NUM_26
#define PIN_GREEN GPIO_NUM_25

#define PIN_ERR GPIO_NUM_33

extern "C" int wait_wifi_connection();

#include <iostream>


typedef unsigned char BYTE;

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(BYTE c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::vector<BYTE> base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  BYTE char_array_4[4], char_array_3[3];
  std::vector<BYTE> ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
          ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }

  return ret;
}


ESPApp::ESPApp(bool autoConnect) : OctopusApp(autoConnect) {
    gpio_reset_pin(PIN_RED);
    gpio_reset_pin(PIN_BLUE);
    gpio_reset_pin(PIN_GREEN);

    gpio_set_direction(PIN_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BLUE, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_GREEN, GPIO_MODE_OUTPUT);
    
    this->_udpPort = 3000;
    // TODO : make _udpPort configurable
}

std::shared_ptr<OctopusConnection> ESPApp::_createConnection(std::string uri) {
    return std::make_shared<ESPConnection>(uri);
}

void ESPApp::_connectionStatusUpdate(bool wasConnected, bool isConnected) {
    OctopusApp::_connectionStatusUpdate(wasConnected, isConnected);

    if(isConnected) {
        gpio_set_level(PIN_ERR, 0);
        std::cout << "connected !" << std::endl;

        this->asyncSendData({
                {"type", "init"},
                {"app", {
                    {"type", "esp32"}
                }}
		});
    } else {
        gpio_set_level(PIN_ERR, 1);
    }
}

void ESPApp::_beginUdp() {
    // TODO : check if connected to wifi, if not, connect to wifi
    int wifi_res = wait_wifi_connection();
    if (wifi_res != 0) {
        ESP_LOGI("app", "Wifi connection failed");
        return;
    }

    OctopusApp::_beginUdp();
}

void ESPApp::_processMessage(Json::json message) {
    try {
        std::cout << "message : " << message.dump() << std::endl;
        std::string type = message["type"];
        if (type == "init") {
            if(message["data"].get<std::string>() == "OK") {
                std::cout << "[LedStrip] Received Server ACK." << std::endl;


                //Poco::
                Json::json subscribeBroadcastData_alarm = {
                    {"type", "core"},
                    {"id", uuid::generate_uuid_v4()},
                    {"content", {
                        {"type", "subscribebroadcast"},
                        {"channel", "alarm"}
                    }}
                };

                this->asyncSendData(subscribeBroadcastData_alarm);

                Json::json subscribeBroadcastData_speechWave = {
                    {"type", "core"},
                    {"id", uuid::generate_uuid_v4()},
                    {"content", {
                        {"type", "subscribebroadcast"},
                        {"channel", "speech_wave"}
                    }}
                };

                this->asyncSendData(subscribeBroadcastData_speechWave);
            } else {
                std::cout << "[Chat LedStrip] Received Server ACK. Error : " << message["data"].get<std::string>() <<  std::endl;
            }
        } else if (type == "broadcast") {
            if (message["channel"].get<std::string>() == "alarm") {
                this->launchLedsWakeUp();
            } else if (message["channel"].get<std::string>() == "speech_wave") {                
                this->launchLedsSpeechWave(message["content"]["play_timestamp"], message["content"]["timestep"], message["content"]["soundwave"]);
            } else {
                std::cout << "Got broadcast message on channel : " << message["channel"].get<std::string>() << std::endl;
                std::cout << "Ingored" << std::endl;
            }
        }
    } catch(Json::json::exception &ex) {
        std::cout << "Error when processing message : " << ex.id << std::endl;
        std::cout << "=> " << ex.what() << std::endl;
    }
}

void ESPApp::launchLedsWakeUp() {
    // TODO : use locker to handle unlock during exceptions
    this->_ledsMutex.lock();

    if(this->_ledsThread != nullptr) {
        ledsRun = false;
        this->_ledsThread->join();
    }

    this->_ledsThread = std::make_shared<std::thread>(&ESPApp::ledsWakeUpLoop, this);

    this->_ledsMutex.unlock();
}

void ESPApp::ledsWakeUpLoop() {
    this->ledsRun = true;

    const int setup_time = 10; 
    const int one_second = 1000000 / 1000;
    const int step = setup_time * one_second / 1000;
    const int total_time = 1200; 

    int pwm = 20;
    int elapsed = 0;
    int second_elapsed = 0;

    std::cout << "begin" << std::endl;
    while(ledsRun) {
        gpio_set_level(PIN_RED, 1);
        gpio_set_level(PIN_BLUE, 1);
        gpio_set_level(PIN_GREEN, 1);
        usleep(pwm);

        if(pwm < 1000) {
            gpio_set_level(PIN_RED, 0);
            gpio_set_level(PIN_BLUE, 0);
            gpio_set_level(PIN_GREEN, 0);
            usleep(1000 - pwm);
            std::cout << pwm << std::endl;
        }
        
        elapsed ++;

        if (pwm < 1000 && elapsed % step == 0){
            pwm ++; // += 10?
        }
        
        if (elapsed >= one_second) {
            second_elapsed ++;
            elapsed = 0;
        }

        if (second_elapsed >= total_time)
            ledsRun = false;
    }

    gpio_set_level(PIN_RED, 0);
    gpio_set_level(PIN_BLUE, 0);
    gpio_set_level(PIN_GREEN, 0);
    std::cout << "end alarm" << std::endl;
}

void ESPApp::launchLedsSpeechWave(int64_t play_timestamp, double timestep, std::string soundwave) {
    // TODO : refactor with launchLedsWakeUp
    this->_ledsMutex.lock();

    if(this->_ledsThread != nullptr) {
        ledsRun = false;
        this->_ledsThread->join();
    }

    this->_ledsThread = std::make_shared<std::thread>(&ESPApp::ledsSpeechWaveLoop, this, play_timestamp, timestep, soundwave);

    this->_ledsMutex.unlock();
}

#define PWM_TOTAL 1000
#define PWM_VARIABLE 950

void ESPApp::ledsSpeechWaveLoop(int64_t play_timestamp, double timestep, std::string soundwave) {
    this->ledsRun = true;
    
    int64_t now = duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::cout << "Current timestamp : " << now << std::endl;
    std::cout << "Play timestamp : " << play_timestamp << std::endl;

    if (now < 1600000000000) {
        // TODO : play immediatly ?
        std::cout << "System time not synchronized" << std::endl;
        return;
    }

    uint32_t pwminterval = PWM_VARIABLE;

    std::vector<BYTE> decoded = base64_decode(soundwave);
    uint64_t total_len = timestep * decoded.size();

    while(ledsRun) {
        now = duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        int64_t diff = now - play_timestamp;

        uint32_t remaining_pwm = pwminterval;

        if (diff < 0) {
        } else if (diff >= total_len) {
            ledsRun = false;
            remaining_pwm = 0;
            continue;
        } else {

            int curr_i = (diff * decoded.size()) / total_len;
            if (curr_i > decoded.size()) {
                std::cerr << "WARNING : Got a byte index > total number of bytes !" << std::endl;
                continue;
            }

            uint32_t sleeptime = ((uint32_t) decoded[curr_i]) * PWM_VARIABLE / 256;
            if (sleeptime < PWM_VARIABLE) 
                remaining_pwm -= sleeptime;

            gpio_set_level(PIN_RED, 1);
            gpio_set_level(PIN_BLUE, 1);
            gpio_set_level(PIN_GREEN, 1);

            usleep(50);
            usleep(sleeptime);
        }

        gpio_set_level(PIN_RED, 0);
        gpio_set_level(PIN_BLUE, 0);
        gpio_set_level(PIN_GREEN, 0);

        // usleep(50);
        usleep(remaining_pwm);
    }

    gpio_set_level(PIN_RED, 0);
    gpio_set_level(PIN_BLUE, 0);
    gpio_set_level(PIN_GREEN, 0);
    std::cout << "end speech_wave" << std::endl;
}