#ifndef ModbeeMPPT_h
#define ModbeeMPPT_h

#include <Arduino.h>
#include <BQ25798.h>
#include <ESP32_SoftWire.h>
#include "ModbeeMpptAPI.h"
#include "ModbeeMpptConfig.h"
#include "ModbeeMpptLog.h" // Include for ModbeeMpptLog
#include "ModbeeMpptPowerSave.h" // Include for ModbeeMpptPowerSave

// Forward declaration to avoid circular dependency
class ModbeeMpptWebServer;

// Define SoftI2C pins
#define SDA_PIN 3
#define SCL_PIN 2

class ModbeeMPPT {
public:
  ModbeeMPPT();
  bool begin(const char* configFile = nullptr);  // Optional custom config file
  void loop();
  
  // LED management functions
  void initializeLEDs(int numLeds = 1, uint8_t brightness = 250);
  void updateLEDs();
  void showInitializationError();  // Show red blinking for init failure
  
  // Status and monitoring functions
  void printStatus();                 // Complete comprehensive status
  void printQuickStatus();           // Essential status only
  void printPowerMeasurements();     // Power and voltage readings
  void printConfiguration();         // Current configuration settings
  void printFaults();               // Fault status and diagnostics
  void printRegisterDebug();        // Raw register values and decoding
  void printComprehensiveBatteryStatus(); // Comprehensive battery voltage and SOC info
  
  // Battery detection and management
  void enableChargingIfBatteryPresent(); // Check for battery and enable charging if found
  
  // Configuration management
  bool saveConfig() { return config.saveConfig(); }
  bool loadConfig() { return config.loadConfig(); }
  bool resetConfig() { return config.resetToDefaults(); }
  
  // Critical settings management
  void setCriticalSettingsUpdateInterval(unsigned long intervalMs) { _criticalSettingsUpdateInterval = intervalMs; }
  unsigned long getCriticalSettingsUpdateInterval() const { return _criticalSettingsUpdateInterval; }
  void applyCriticalSettingsNow() { applyCriticalSettings(); }  // Force immediate update
  
  // Web server management
  void initWebServer();
  void enableWebServer();
  bool isWebServerEnabled();
  // Low-power boot state
  bool isLowPowerBoot() const { return _lowPowerBootMode; }
  
  bool hasFaults();
  String getChargeStateString();

  BQ25798 _bq25798;
  ModbeeMpptAPI api;  // Single API instance - public for easy access
  ModbeeMpptConfig config;  // Configuration manager - public for easy access
  ModbeeMpptLog statsLog;   // Persistent stats manager - public for easy access
  ModbeeMpptPowerSave powerSave; // Power management module - public for easy access
  ModbeeMpptWebServer* _webServer; // Web server instance - public for easy access

  // LED management (made public for direct access by power save)
  CRGB* _leds;
  int _numLeds;
  uint8_t _ledBrightness;
  bool _ledsInitialized;
  unsigned long _lastErrorBlink;
  bool _errorBlinkState;
  float _cachedSOC = 0.0f;  // updated on SOC interval only
private:
  SoftWire _i2c;

  bool _batteryPresent; // Tracks battery presence, private member

  bool _webServerEnabled;
  // Configuration intervals (loaded from config)
  unsigned long _batteryCheckInterval;
  unsigned long _socCheckInterval;
  unsigned long _ledUpdateInterval;
  unsigned long _criticalSettingsUpdateInterval;  // Interval to re-apply critical settings (hardcoded)
  unsigned long _configApplyInterval;  // Interval to re-apply config settings (user-configurable)
  // Low-power boot controls
  bool _lowPowerBootMode;
  float _lowPowerSocThreshold = 5.0f;   // % SOC considered "flat" at boot
  uint32_t _lowPowerSleepMs = 30000;    // Light sleep interval during recovery
  
  // Helper functions
  void applyCriticalSettings();  // Re-apply watchdog, HIZ, ADC settings (not user-configurable)
};

#endif
