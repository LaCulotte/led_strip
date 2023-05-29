#include "esp_app.hpp"

#include <iostream>

ESPApp* app = nullptr;

// void onMessage(Json::json message) {
//     std::cout << "message : " << message.dump() << std::endl;
// }

// void onConnected(bool b) {
//     std::cout << "connected !" << std::endl;

//     app->sendData({
// 			{"type", "init"},
// 			{"app", {
// 				{"type", "esp32"}
// 			}}
// 		});
// }

extern "C" void start_app(void) {
    app = new ESPApp(true);
    app->start();
    // app->connect("ws://192.168.1.43:8000/");
    // app = new OctopusApp("ws://192.168.1.43:8000/");

    // app->setOnConnected(onConnected);
    // app->setOnMessage(onMessage);

    // vTaskDelay(1000);
    // app->close();

    // app->connect();
}