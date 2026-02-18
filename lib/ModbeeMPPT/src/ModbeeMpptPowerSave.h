#ifndef MODBEE_MPPT_POWERSAVE_H
#define MODBEE_MPPT_POWERSAVE_H

#include <Arduino.h>

// WiFi Button Pin (GPIO0 on ESP32-C3)
#define WIFI_BUTTON_PIN 0

class ModbeeMPPT;

class ModbeeMpptPowerSave {
public:
    ModbeeMpptPowerSave(ModbeeMPPT& mppt);
    void begin();
    void loop();
    void handleWiFiButton();
    void checkPowerSave();
    void enterLightSleep(uint32_t wakeTimeMs);
    void enableWiFi();
    void disableWiFi();
    void enableBluetooth();
    void disableBluetooth();
    void setPowerSaveMode(uint8_t mode);
    void setSocSetpoint(float soc);
    void setSocSetpoint2(float soc);
    void setWakeInterval(uint32_t intervalMs);
    void setWakeInterval2(uint32_t intervalMs);

private:
    ModbeeMPPT& _mppt;
    uint8_t _powerSaveMode;
    float _socSetpoint1;
    float _socSetpoint2;
    uint32_t _wakeInterval1;
    uint32_t _wakeInterval2;
    unsigned long _lastSocCheck;
    unsigned long _wifiEnableTime;
    bool _wifiActive;
    bool _bluetoothActive;
    bool _buttonPressed;
};

#endif // MODBEE_MPPT_POWERSAVE_H
