#include "ModbeeMpptPowerSave.h"
#include "ModbeeMPPT.h"
#include "ModbeeMpptWebServer.h"
#include <esp_sleep.h>

ModbeeMpptPowerSave::ModbeeMpptPowerSave(ModbeeMPPT& mppt)
    : _mppt(mppt), _powerSaveMode(1), _socSetpoint1(20.0), _socSetpoint2(10.0),
      _wakeInterval1(10000), _wakeInterval2(600000), _lastSocCheck(0),
      _wifiEnableTime(0), _bluetoothActive(false), _buttonPressed(false) {}

void ModbeeMpptPowerSave::begin() {
    pinMode(WIFI_BUTTON_PIN, INPUT);
    _wifiEnableTime = millis();
}

void ModbeeMpptPowerSave::loop() {
    unsigned long now = millis();
    // Directly trigger power management actions on events
    // WiFi timeout (5 min)
    if (now - _wifiEnableTime > 300000) {
        disableWiFi();
        disableBluetooth();
    }
    // Button press triggers enableWiFi directly (see handleWiFiButton)
    handleWiFiButton();
    // SOC check triggers sleep directly
    if (now - _lastSocCheck > 60000) {
        _lastSocCheck = now;
        checkPowerSave();
    }
}

void ModbeeMpptPowerSave::handleWiFiButton() {
    // Simple debounce logic
    static unsigned long lastPressTime = 0;
    static bool lastButtonState = false;
    bool buttonState = digitalRead(WIFI_BUTTON_PIN) == HIGH;
    if (buttonState && !lastButtonState) {
        lastPressTime = millis();
    }
    if (!buttonState && lastButtonState) {
        if (millis() - lastPressTime > 50) { // debounce 50ms
            // Button was pressed, trigger WiFi enable directly
            // Ensure web server is initialized and enabled, and start WiFi regardless of low-power
            if (!_mppt._webServer) {
                _mppt.initWebServer();
            }
            if (!_mppt.isWebServerEnabled()) {
                _mppt.enableWebServer();
            }
            enableWiFi();
            _wifiEnableTime = millis();
        }
    }
    lastButtonState = buttonState;
}

void ModbeeMpptPowerSave::checkPowerSave() {
    float soc = _mppt.api.getActualBatterySOC();
    if (_powerSaveMode == 1 && soc < _socSetpoint1) {
        enterLightSleep(_wakeInterval1);
    } else if (_powerSaveMode == 2 && soc < _socSetpoint2) {
        enterLightSleep(_wakeInterval2);
    }
}

void ModbeeMpptPowerSave::enterLightSleep(uint32_t wakeTimeMs) {
    // Prepare for light sleep
    esp_sleep_enable_timer_wakeup(wakeTimeMs * 1000); // Convert ms to us
    // Optionally enable wakeup by button GPIO
    //esp_sleep_enable_ext0_wakeup((gpio_num_t)WIFI_BUTTON_PIN, 1);
    // Enable wakeup by button GPIO using ext1 (ESP32-C3)
    //esp_sleep_enable_ext1_wakeup(1ULL << WIFI_BUTTON_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);
    //esp_deep_sleep_enable_gpio_wakeup(GPIO_NUM_0, ESP_GPIO_WAKEUP_GPIO_HIGH);
    gpio_wakeup_enable((gpio_num_t)WIFI_BUTTON_PIN, GPIO_INTR_HIGH_LEVEL); // Wake on button HIGH
    esp_sleep_enable_gpio_wakeup();
    // Turn off LED before sleep
    _mppt._leds[0] = CRGB::Black;
    FastLED.show();
    disableWiFi();
    disableBluetooth();
    esp_light_sleep_start();
    // After wakeup, re-enable WiFi if needed
}

void ModbeeMpptPowerSave::enableWiFi() {
    // Enable WiFi fully for network use
    setCpuFrequencyMhz(160);
    // Ensure web server exists
    if (!_mppt._webServer) {
        _mppt.initWebServer();
    }
    if (_mppt._webServer) {
        _mppt._webServer->startWiFi();
    }
    _wifiEnableTime = millis();
}

void ModbeeMpptPowerSave::disableWiFi() {
    // Disable WiFi fully for power saving
    if (_mppt._webServer) {
        _mppt._webServer->stopWiFi();
    }
    setCpuFrequencyMhz(80);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF); // ESP-IDF/Arduino API
}

void ModbeeMpptPowerSave::enableBluetooth() {
    _bluetoothActive = true;
    btStart(); // ESP-IDF API, if available for ESP32-C3
}

void ModbeeMpptPowerSave::disableBluetooth() {
    _bluetoothActive = false;
    btStop(); // ESP-IDF API, if available for ESP32-C3

}

void ModbeeMpptPowerSave::setPowerSaveMode(uint8_t mode) {
    _powerSaveMode = mode;
}

void ModbeeMpptPowerSave::setSocSetpoint(float soc) {
    _socSetpoint1 = soc;
}

void ModbeeMpptPowerSave::setSocSetpoint2(float soc) {
    _socSetpoint2 = soc;
}

void ModbeeMpptPowerSave::setWakeInterval(uint32_t intervalMs) {
    _wakeInterval1 = intervalMs;
}

void ModbeeMpptPowerSave::setWakeInterval2(uint32_t intervalMs) {
    _wakeInterval2 = intervalMs;
}
