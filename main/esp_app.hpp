#ifndef ESP_OCTOPUS_APP
#define ESP_OCTOPUS_APP

#include <octopus-utils/octopus-app/app.hpp>
#include <esp_connection.hpp>

class ESPApp : public OctopusApp {
public:
    ESPApp(bool autoConnect = false);
    
protected:
    virtual void _beginUdp();

    virtual std::shared_ptr<OctopusConnection> _createConnection(std::string uri) override;

    virtual void _connectionStatusUpdate(bool wasConnected, bool isConnected) override;
    virtual void _processMessage(Json::json message) override;

    void launchLedsWakeUp();
    void ledsWakeUpLoop();

    void launchLedsSpeechWave(int64_t play_timestamp, double timestep, std::string soundwave);
    void ledsSpeechWaveLoop(int64_t play_timestamp, double timestep, std::string soundwave);

    bool ledsRun = false;
    std::shared_ptr<std::thread> _ledsThread;
    std::mutex _ledsMutex;

};

#endif