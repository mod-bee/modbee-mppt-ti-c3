#include "ModbeeMPPT.h"
#include "ModbeeMpptAPI.h"
#include "ModbeeMpptDebug.h"
#include "ModbeeMpptWebServer.h"
#include "ModbeeMpptLog.h"

ModbeeMPPT::ModbeeMPPT()
  : _bq25798(&_i2c),
    api(*this),
    config(),
    statsLog(&api),
    powerSave(*this),
    _webServer(nullptr),
    _webServerEnabled(false),
  _lowPowerBootMode(false),
  _cachedSOC(0.0f),
    _leds(nullptr),
    _numLeds(0),
    _ledBrightness(250),
    _ledsInitialized(false),
    _lastErrorBlink(0),
    _errorBlinkState(false),
    _batteryPresent(false)
{
  // Load default intervals (will be overridden when config loads)
  _batteryCheckInterval = 30000;
  _socCheckInterval = 60000;
  _ledUpdateInterval = 1000;
  _criticalSettingsUpdateInterval = 60000;  // Re-apply critical settings every 60 seconds (hardcoded)
  _configApplyInterval = 300000;  // Re-apply user-configurable settings every 5 minutes (default)
}

bool ModbeeMPPT::begin(const char* configFile) {
  // Always boot radios off and at 80MHz to minimize consumption until we decide otherwise
  powerSave.disableWiFi();
  powerSave.disableBluetooth();
  // Initialize I2C and BQ25798
  _i2c.begin(SDA_PIN, SCL_PIN);
  if (!_bq25798.begin(BQ25798_I2C_ADDRESS)) {
    return false;
  }
  
  // Initialize configuration system
  if (!config.begin()) {
    Serial.println("Warning: Failed to initialize config system, using defaults");
  }
  
  // Load intervals from configuration
  const auto& configData = config.data;
  _batteryCheckInterval = configData.battery_check_interval;
  _socCheckInterval = configData.soc_check_interval;
  _ledUpdateInterval = 1000;  // Hardcoded 1 second LED update interval

  // Apply critical non-user-configurable settings (ADC, watchdog, HIZ)
  applyCriticalSettings();
  
  // Apply all user-configurable settings from config (including MPPT)
  if (!config.applyToMPPT(api)) {
    Serial.println("Warning: Failed to apply configuration, using current settings");
  }
  
  // Perform battery detection before enabling charging
  _batteryPresent = api.detectBatteryConnected();
  float bootSoc = api.getActualBatterySOC();
  _cachedSOC = bootSoc; // seed cached SOC at boot
  bool flatOrMissing = (!_batteryPresent) || (bootSoc < _lowPowerSocThreshold);
  if (flatOrMissing) {
    // Force charging path to recover system. Ensure charge path enabled and HIZ off.
    api.setHIZMode(false);
    api.setChargeEnable(true);
    _lowPowerBootMode = true;
  } else {
    // Normal behavior: enable charging based on detection
    api.setChargeEnable(_batteryPresent);
  }
  
  api.updateTrueBatteryVoltage();
  
  Serial.println("MPPT initialized with configuration:");
  config.printConfig();
  
  statsLog.begin();
  statsLog.loadStatsToAPI();

  powerSave.begin();
  
  return true;
}

/*
void ModbeeMPPT::enableChargingIfBatteryPresent() {
  // Convenient function to check for battery and enable charging if found
  _batteryPresent = api.detectBatteryConnected();
  if (_batteryPresent) {
    api.setChargeEnable(true);
  } else {
    api.setChargeEnable(false);
  }
}
*/

void ModbeeMPPT::loop() {
  // Periodically check for battery connection changes
  static unsigned long lastBatteryCheck = 0;
  // SOC check interval - only when charging
  static unsigned long lastSOCCheck = 0;
  // LED update interval
  static unsigned long lastLEDUpdate = 0;
  // Critical settings update interval (re-apply watchdog, HIZ, ADC settings)
  static unsigned long lastCriticalSettingsUpdate = 0;
  // Config settings re-apply interval
  static unsigned long lastConfigApply = 0;
  // Stats update interval
  static unsigned long lastStatsUpdate = 0;
  static unsigned long lastStatsSave = 0;
  unsigned long currentTime = millis();

  // Update API state machines (including true battery voltage)
  if (currentTime - lastStatsUpdate >= 1000) { // 1 second interval
    lastStatsUpdate = currentTime;
    api.updateStats();
  }
  api.update();
  
  // Battery connection and charge enable logic (using configurable interval)
  if (currentTime - lastBatteryCheck >= _batteryCheckInterval) {
    lastBatteryCheck = currentTime;
    // Only check for battery presence if NOT actually charging
    if (!api.isCharging()) {
      _batteryPresent = api.detectBatteryConnected();
      if (_batteryPresent) {
        api.setChargeEnable(true);
      } else {
        //api.setChargeEnable(false);
      }
    }
    // If charging, do not toggle anything here; let charging run unless a fault is detected elsewhere
  }
  
  // SOC measurement optimization (using configurable interval)
  // - Only perform complex true battery voltage measurement when charging
  // - When not charging, getTrueBatteryVoltage() returns current VBAT directly
  // - Reduced frequency to minimize charge interruptions
  if (currentTime - lastSOCCheck >= _socCheckInterval) {
    lastSOCCheck = currentTime;
    
    // Only update true battery voltage measurement if we are actually charging
    if (api.isCharging()) {
      api.updateTrueBatteryVoltage();
    }
  // Update cached SOC using latest available measurements
  _cachedSOC = api.getActualBatterySOC();
    // If not charging, the true battery voltage will just return the current VBAT reading
  }
  
  // Update LEDs based on current status (using configurable interval)
  if (currentTime - lastLEDUpdate >= _ledUpdateInterval) {
    lastLEDUpdate = currentTime;
    updateLEDs();
  }

  // If in low-power boot (flat/missing battery at boot), do light-sleep cycles until charging
  if (_lowPowerBootMode) {
  // Avoid auto-starting webserver/WiFi, but do NOT override a user button enable
    // Once charger reports charging current, exit low-power boot
    if (api.isCharging()) {
      // Still keep radios off unless user enables via wifi button; stay frugal
      // Optionally can clear low-power boot when SOC rises enough
  // Use cached SOC updated on interval to avoid repeated SOC computations
  if (_cachedSOC > (_lowPowerSocThreshold + 3.0f)) {
        _lowPowerBootMode = false;
      }
    } else {
      // Enter light sleep for a short interval to reduce draw while BQ starts up
      //powerSave.enterLightSleep(_lowPowerSleepMs);
    }
  }
  
  // Re-apply critical settings periodically (watchdog, HIZ, ADC)
  // This ensures the BQ25798 stays properly configured even if it resets itself
  if (currentTime - lastCriticalSettingsUpdate >= _criticalSettingsUpdateInterval) {
    lastCriticalSettingsUpdate = currentTime;
    applyCriticalSettings();
  }
  
  // Periodically re-apply config settings to BQ25798
  if (currentTime - lastConfigApply >= _configApplyInterval) {
    lastConfigApply = currentTime;
    config.applyToMPPT(api);
  }
  
  // Save stats to JSON every 5 minutes
  if (currentTime - lastStatsSave >= 300000) {
    lastStatsSave = currentTime;
    statsLog.saveStatsFromAPI();
  }
  
  // Update web server if enabled
  if (_webServerEnabled && _webServer) {
    _webServer->loop();
  }

  // Power management
  powerSave.loop();
}

void ModbeeMPPT::printStatus() {
  ModbeeMpptDebug debug(*this);
  debug.printCompleteStatus();
}

void ModbeeMPPT::printQuickStatus() {
  ModbeeMpptDebug debug(*this);
  debug.printPowerMeasurements();
  Serial.println();
  debug.printStatus();
  Serial.println();
  debug.printFaults();
}

void ModbeeMPPT::printPowerMeasurements() {
  ModbeeMpptDebug debug(*this);
  debug.printPowerMeasurements();
}

void ModbeeMPPT::printConfiguration() {
  ModbeeMpptDebug debug(*this);
  debug.printConfiguration();
}

void ModbeeMPPT::printFaults() {
  ModbeeMpptDebug debug(*this);
  debug.printFaults();
}

void ModbeeMPPT::printRegisterDebug() {
  ModbeeMpptDebug debug(*this);
  debug.printRawRegisters();
  Serial.println();
  debug.printRegisterDecoding();
}

void ModbeeMPPT::printComprehensiveBatteryStatus() {
  ModbeeMpptDebug debug(*this);
  debug.printComprehensiveBatteryStatus();
}

bool ModbeeMPPT::hasFaults() {
  return api.hasFaults();
}

String ModbeeMPPT::getChargeStateString() {
  return api.getChargeStateString();
}

void ModbeeMPPT::initializeLEDs(int numLeds, uint8_t brightness) {
  _numLeds = numLeds;
  _ledBrightness = brightness;
  
  // Allocate memory for LED array
  _leds = new CRGB[_numLeds];
  
  // Initialize FastLED with pin 10 (hardcoded for this application)
  FastLED.addLeds<WS2812, 10, GRB>(_leds, _numLeds);

  // Set initial state (orange faded)
  _leds[0] = CRGB::Orange;
  _leds[0].fadeToBlackBy(_ledBrightness);
  FastLED.show();
  
  _ledsInitialized = true;
  Serial.printf("LEDs initialized: brightness=%d\n", _ledBrightness);
}

void ModbeeMPPT::updateLEDs() {
  if (!_ledsInitialized || _leds == nullptr) {
    return;  // LEDs not initialized
  }
  
  // Update LED based on status
  if (hasFaults()) {
    _leds[0] = CRGB::Red;  // Fault
    _leds[0].fadeToBlackBy(_ledBrightness);
  } else if (api.isCharging()) {
    _leds[0] = CRGB::Yellow;  // Charging
    _leds[0].fadeToBlackBy(_ledBrightness);
  } else {
    // Check if charge is complete by looking at charge state
    String state = getChargeStateString();
    if (state == "Charge Done") {
      _leds[0] = CRGB::Green;   // Charge complete
      _leds[0].fadeToBlackBy(_ledBrightness);
    } else {
      _leds[0] = CRGB::Blue;    // Not charging
      _leds[0].fadeToBlackBy(_ledBrightness);
    }
  }
  
  FastLED.show();
}

void ModbeeMPPT::showInitializationError() {
  if (!_ledsInitialized || _leds == nullptr) {
    return;  // LEDs not initialized
  }
  
  // Non-blocking blink red to indicate initialization error
  unsigned long currentTime = millis();
  if (currentTime - _lastErrorBlink >= 500) {
    _lastErrorBlink = currentTime;
    _errorBlinkState = !_errorBlinkState;
    
    if (_errorBlinkState) {
      _leds[0] = CRGB::Red;
    } else {
      _leds[0] = CRGB::Black;
    }
    FastLED.show();
  }
}

// Web Server Management
void ModbeeMPPT::initWebServer() {
  if (!_webServer) {
    _webServer = new ModbeeMpptWebServer(*this);
    _webServer->begin();
    // Do not auto-enable WiFi if we are in low-power boot
    _webServerEnabled = !_lowPowerBootMode;
    if (_webServerEnabled) {
      Serial.println("Web server initialized and starting WiFi AP...");
    } else {
      Serial.println("Web server initialized but WiFi kept OFF due to low-power boot");
    }
  }
}

void ModbeeMPPT::enableWebServer() {
  if (!_webServer) {
    initWebServer();
  }
  if (_webServer && !_webServerEnabled) {
    _webServerEnabled = true;
    Serial.println("Web server enabled via button press.");
    // If WiFi was previously kept off (e.g., low-power boot), start it now
    _webServer->startWiFi();
  }
}

bool ModbeeMPPT::isWebServerEnabled() {
  return _webServerEnabled;
}

void ModbeeMPPT::applyCriticalSettings() {
  // **CRITICAL SYSTEM SETTINGS - NOT USER CONFIGURABLE**
  // These settings must be periodically re-applied because the BQ25798
  // may reset them due to faults, power cycles, or other conditions
  
  // Configure ADC for continuous operation with maximum resolution
  api.configureADC(MODBEE_ADC_RES_15BIT, MODBEE_ADC_AVG_1, MODBEE_ADC_CONTINUOUS);
  api.setWatchdogEnable(false);
  api.setHIZMode(false);
  api.setBackupMode(false); 
  api.setPWMFrequency(MODBEE_PWM_FREQ_1_5MHZ);
  // Disable ICO to allow MPPT to function
  api.setICOEnable(false);
  // Configure system settings (not user-configurable)
  api.setBatteryDischargeSenseEnable(true);  // Always enable discharge current sensing
}
