/*!
 * @file ModbeeMpptConfig.cpp
 * 
 * @brief Implementation of ModbeeMPPT configuration management
 */

#include "ModbeeMpptConfig.h"

ModbeeMpptConfig::ModbeeMpptConfig() : _initialized(false) {
  setDefaults();
}

bool ModbeeMpptConfig::begin() {
  if (!LittleFS.begin()) {
    Serial.println("Failed to initialize LittleFS");
    return false;
  }
  
  if (!ensureConfigDirectory()) {
    Serial.println("Failed to create config directory");
    return false;
  }
  
  _initialized = true;
  
  // Try to load existing config, use defaults if not found
  if (!loadConfig()) {
    Serial.println("No existing config found, using defaults");
    return saveConfig(); // Save defaults
  }
  
  return true;
}

bool ModbeeMpptConfig::loadConfig() {
  if (!_initialized) return false;
  
  if (!LittleFS.exists(MODBEE_CONFIG_FILE)) {
    Serial.println("Config file does not exist");
    return false;
  }
  
  File file = LittleFS.open(MODBEE_CONFIG_FILE, "r");
  if (!file) {
    Serial.println("Failed to open config file for reading");
    return false;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.printf("Failed to parse config JSON: %s\n", error.c_str());
    return false;
  }
  
  bool success = loadFromJson(doc);
  if (success && validateConfig()) {
    Serial.println("Configuration loaded successfully");
    return true;
  } else {
    Serial.println("Invalid configuration loaded, reverting to defaults");
    setDefaults();
    return false;
  }
}

bool ModbeeMpptConfig::saveConfig() {
  if (!_initialized) return false;
  
  if (!validateConfig()) {
    Serial.println("Cannot save invalid configuration");
    return false;
  }
  
  JsonDocument doc;
  if (!saveToJson(doc)) {
    Serial.println("Failed to create JSON document");
    return false;
  }
  
  File file = LittleFS.open(MODBEE_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  
  size_t bytesWritten = serializeJsonPretty(doc, file);
  file.close();
  
  if (bytesWritten == 0) {
    Serial.println("Failed to write config file");
    return false;
  }
  
  Serial.printf("Configuration saved (%d bytes)\n", bytesWritten);
  return true;
}

bool ModbeeMpptConfig::resetToDefaults() {
  setDefaults();
  return saveConfig();
}

bool ModbeeMpptConfig::applyToMPPT(ModbeeMpptAPI& api) {
  if (!validateConfig()) return false;
  
  // Battery configuration
  api.setBatteryType(data.battery_type, data.battery_cell_count);
  api.setChargeVoltage(data.charge_voltage);
  api.setChargeCurrent(data.charge_current);
  api.setMinSystemVoltage(data.min_system_voltage);
  
  // Charging control
  api.setTerminationCurrent(data.termination_current);
  api.setRechargeThreshold(data.recharge_threshold);
  api.setPrechargeCurrent(data.precharge_current);
  api.setPrechargeVoltageThreshold(data.precharge_voltage_threshold);
  
  // Input limits & protection
  api.setInputVoltageLimit(data.input_voltage_limit);
  api.setInputCurrentLimit(data.input_current_limit);
  api.setVACOVP(data.vac_ovp_threshold);
  
  // Timer configuration
  api.setFastChargeTimerEnable(data.fast_charge_timer_enable);
  api.setFastChargeTimer(data.fast_charge_timer);
  api.setPrechargeTimerEnable(data.precharge_timer_enable);
  api.setPrechargeTimer(data.precharge_timer);
  api.setTopOffTimer(data.topoff_timer);
  
  // MPPT configuration
  api.setMPPTVOCPercent(data.mppt_voc_percent);
  api.setMPPTVOCDelay(data.mppt_voc_delay);
  api.setMPPTVOCRate(data.mppt_voc_rate);
  api.setMPPTEnable(data.mppt_enable);
  
  // Power Management & Noise Control
  api.setForwardPFM(data.pfm_forward_enable);
  api.setForwardOOA(data.ooa_forward_enable);
  
  Serial.println("Configuration applied to MPPT successfully");
  return true;
}

void ModbeeMpptConfig::setDefaults() {
  // Battery Configuration - Default for 3S Li-Ion
  data.battery_type = MODBEE_BATTERY_LIPO;
  data.battery_cell_count = 3;
  data.charge_voltage = 12.6f;
  data.charge_current = 1.0f;
  data.min_system_voltage = 10.0f;
  
  // Charging Control - Optimized for old batteries to prevent flickering
  data.termination_current = 0.12f;  // 120mA actual (compensated for library bug)
  data.recharge_threshold = 0.4f;    // 400mV for better hysteresis
  data.precharge_current = 0.2f;     // 200mA
  data.precharge_voltage_threshold = MODBEE_VBAT_LOWV_71_4_PERCENT;
  
  // Input Limits & Protection
  data.input_voltage_limit = 22.0f;
  data.input_current_limit = 3.0f;
  data.vac_ovp_threshold = 26.0f;
  
  // Timer Configuration
  data.fast_charge_timer_enable = true;
  data.fast_charge_timer = MODBEE_TIMER_12HR;
  data.precharge_timer_enable = true;
  data.precharge_timer = MODBEE_PRECHARGE_TIMER_2HR;  // 2 hour default
  data.topoff_timer = MODBEE_TOPOFF_TIMER_15MIN;
  
  // MPPT Configuration
  data.mppt_voc_percent = MODBEE_VOC_PERCENT_87_5;    // 87.5% VOC for optimal MPPT
  data.mppt_voc_delay = MODBEE_VOC_DELAY_300MS;       // 300ms delay (default)
  data.mppt_voc_rate = MODBEE_VOC_RATE_30S;           // Check VOC every 30 seconds
  data.mppt_enable = true;                            // Enable MPPT
  
  // Power Management & Noise Control
  data.pfm_forward_enable = false;                    // Disable PFM for quiet operation
  data.ooa_forward_enable = true;                     // Enable OOA to maintain 25kHz minimum
  
  // Loop Intervals (in milliseconds)
  data.battery_check_interval = 30000;  // 30 seconds
  data.soc_check_interval = 60000;      // 60 seconds
  data.config_apply_interval = 300000;  // 5 minutes default for config re-apply
}

bool ModbeeMpptConfig::loadFromJson(const JsonDocument& doc) {
  // Battery Configuration
  data.battery_type = static_cast<modbee_battery_type_t>(doc["battery"]["type"] | MODBEE_BATTERY_LIPO);
  data.battery_cell_count = doc["battery"]["cell_count"] | 3;
  data.charge_voltage = doc["battery"]["charge_voltage"] | 12.6f;
  data.charge_current = doc["battery"]["charge_current"] | 1.0f;
  data.min_system_voltage = doc["battery"]["min_system_voltage"] | 9.0f;
  
  // Charging Control
  data.termination_current = doc["charging"]["termination_current"] | 0.12f;
  data.recharge_threshold = doc["charging"]["recharge_threshold"] | 0.4f;
  data.precharge_current = doc["charging"]["precharge_current"] | 0.8f;
  data.precharge_voltage_threshold = static_cast<modbee_vbat_lowv_t>(doc["charging"]["precharge_voltage_threshold"] | MODBEE_VBAT_LOWV_71_4_PERCENT);
  
  // Input Limits & Protection
  data.input_voltage_limit = doc["input"]["voltage_limit"] | 22.0f;
  data.input_current_limit = doc["input"]["current_limit"] | 3.0f;
  data.vac_ovp_threshold = doc["input"]["vac_ovp_threshold"] | 26.0f;
  
  // Timer Configuration
  data.fast_charge_timer_enable = doc["timers"]["fast_charge_enable"] | true;
  data.fast_charge_timer = static_cast<modbee_charge_timer_t>(doc["timers"]["fast_charge_timer"] | MODBEE_TIMER_12HR);
  data.precharge_timer_enable = doc["timers"]["precharge_enable"] | true;
  data.precharge_timer = static_cast<modbee_precharge_timer_t>(doc["timers"]["precharge_timer"] | MODBEE_PRECHARGE_TIMER_2HR);
  data.topoff_timer = static_cast<modbee_topoff_timer_t>(doc["timers"]["topoff_timer"] | MODBEE_TOPOFF_TIMER_15MIN);
  
  // MPPT Configuration
  data.mppt_voc_percent = static_cast<modbee_voc_percent_t>(doc["mppt"]["voc_percent"] | MODBEE_VOC_PERCENT_87_5);
  data.mppt_voc_delay = static_cast<modbee_voc_delay_t>(doc["mppt"]["voc_delay"] | MODBEE_VOC_DELAY_300MS);
  data.mppt_voc_rate = static_cast<modbee_voc_rate_t>(doc["mppt"]["voc_rate"] | MODBEE_VOC_RATE_30S);
  data.mppt_enable = doc["mppt"]["enable"] | true;
  
  // Power Management & Noise Control
  data.pfm_forward_enable = doc["power"]["pfm_forward_enable"] | false;  // Default: disable PFM for quiet operation
  data.ooa_forward_enable = doc["power"]["ooa_forward_enable"] | true;   // Default: enable OOA for noise reduction
  
  // Loop Intervals
  data.battery_check_interval = doc["intervals"]["battery_check"] | 30000UL;
  data.soc_check_interval = doc["intervals"]["soc_check"] | 60000UL;
  data.config_apply_interval = doc["intervals"]["config_apply"] | 60000UL;
  
  return true;
}

bool ModbeeMpptConfig::saveToJson(JsonDocument& doc) const {
  // Battery Configuration
  doc["battery"]["type"] = data.battery_type;
  doc["battery"]["cell_count"] = data.battery_cell_count;
  doc["battery"]["charge_voltage"] = data.charge_voltage;
  doc["battery"]["charge_current"] = data.charge_current;
  doc["battery"]["min_system_voltage"] = data.min_system_voltage;
  
  // Charging Control
  doc["charging"]["termination_current"] = data.termination_current;
  doc["charging"]["recharge_threshold"] = data.recharge_threshold;
  doc["charging"]["precharge_current"] = data.precharge_current;
  doc["charging"]["precharge_voltage_threshold"] = data.precharge_voltage_threshold;
  
  // Input Limits & Protection
  doc["input"]["voltage_limit"] = data.input_voltage_limit;
  doc["input"]["current_limit"] = data.input_current_limit;
  doc["input"]["vac_ovp_threshold"] = data.vac_ovp_threshold;
  
  // Timer Configuration
  doc["timers"]["fast_charge_enable"] = data.fast_charge_timer_enable;
  doc["timers"]["fast_charge_timer"] = data.fast_charge_timer;
  doc["timers"]["precharge_enable"] = data.precharge_timer_enable;
  doc["timers"]["precharge_timer"] = data.precharge_timer;
  doc["timers"]["topoff_timer"] = data.topoff_timer;
  
  // MPPT Configuration
  doc["mppt"]["voc_percent"] = data.mppt_voc_percent;
  doc["mppt"]["voc_delay"] = data.mppt_voc_delay;
  doc["mppt"]["voc_rate"] = data.mppt_voc_rate;
  doc["mppt"]["enable"] = data.mppt_enable;
  
  // Power Management & Noise Control
  doc["power"]["pfm_forward_enable"] = data.pfm_forward_enable;
  doc["power"]["ooa_forward_enable"] = data.ooa_forward_enable;
  
  // Loop Intervals
  doc["intervals"]["battery_check"] = data.battery_check_interval;
  doc["intervals"]["soc_check"] = data.soc_check_interval;
  doc["intervals"]["config_apply"] = data.config_apply_interval;
  
  // Add metadata
  doc["version"] = "1.0";
  doc["generated"] = millis();
  
  return true;
}

bool ModbeeMpptConfig::ensureConfigDirectory() {
  // Create /config directory if it doesn't exist
  if (!LittleFS.exists("/config")) {
    return LittleFS.mkdir("/config");
  }
  return true;
}

bool ModbeeMpptConfig::validateConfig() const {
  return validateBatteryConfig() && 
         validateChargingConfig() && 
         validateInputConfig() && 
         validateTimerConfig() && 
         validateIntervalConfig();
}

bool ModbeeMpptConfig::validateBatteryConfig() const {
  if (data.battery_cell_count < 1 || data.battery_cell_count > 4) return false;
  if (data.charge_voltage < 3.0f || data.charge_voltage > 18.8f) return false;
  if (data.charge_current < 0.1f || data.charge_current > 5.0f) return false;
  if (data.min_system_voltage < 2.0f || data.min_system_voltage > 18.8f) return false;
  return true;
}

bool ModbeeMpptConfig::validateChargingConfig() const {
  if (data.termination_current < 0.04f || data.termination_current > 1.0f) return false;
  if (data.recharge_threshold < 0.05f || data.recharge_threshold > 0.8f) return false;
  if (data.precharge_current < 0.04f || data.precharge_current > 2.0f) return false;
  return true;
}

bool ModbeeMpptConfig::validateInputConfig() const {
  if (data.input_voltage_limit < 3.6f || data.input_voltage_limit > 22.0f) return false;
  if (data.input_current_limit < 0.1f || data.input_current_limit > 3.25f) return false;
  if (data.vac_ovp_threshold < 6.0f || data.vac_ovp_threshold > 26.0f) return false;
  return true;
}

bool ModbeeMpptConfig::validateTimerConfig() const {
  // Enum values are inherently valid if within defined range
  return true;
}

bool ModbeeMpptConfig::validateIntervalConfig() const {
  if (data.battery_check_interval < 1000 || data.battery_check_interval > 300000) return false;  // 1s to 5min
  if (data.soc_check_interval < 5000 || data.soc_check_interval > 600000) return false;         // 5s to 10min
  if (data.config_apply_interval < 1000 || data.config_apply_interval > 600000) return false;   // 1s to 10min
  return true;
}
bool ModbeeMpptConfig::updateConfigApplyInterval(unsigned long interval) {
  if (interval < 1000 || interval > 600000) return false;
  data.config_apply_interval = interval;
  return saveConfig();
}

void ModbeeMpptConfig::printConfig() const {
  Serial.println("=== MPPT Configuration ===");
  Serial.printf("Battery Type: %d, Cells: %d\n", data.battery_type, data.battery_cell_count);
  Serial.printf("Charge: %.2fV, %.2fA\n", data.charge_voltage, data.charge_current);
  Serial.printf("Termination: %.3fA, Recharge: %.3fV\n", data.termination_current, data.recharge_threshold);
  Serial.printf("Input Limits: %.1fV, %.2fA\n", data.input_voltage_limit, data.input_current_limit);
  Serial.printf("Intervals: Battery=%lums, SOC=%lums\n", 
                data.battery_check_interval, data.soc_check_interval);
}

String ModbeeMpptConfig::getConfigAsString() const {
  JsonDocument doc;
  saveToJson(doc);
  String output;
  serializeJsonPretty(doc, output);
  return output;
}

// Individual parameter setters with validation
bool ModbeeMpptConfig::setBatteryType(modbee_battery_type_t type, uint8_t cell_count) {
  if (cell_count < 1 || cell_count > 4) return false;
  data.battery_type = type;
  data.battery_cell_count = cell_count;
  return true;
}

bool ModbeeMpptConfig::setChargeVoltage(float voltage) {
  if (voltage < 3.0f || voltage > 18.8f) return false;
  data.charge_voltage = voltage;
  return true;
}

bool ModbeeMpptConfig::setChargeCurrent(float current) {
  if (current < 0.1f || current > 5.0f) return false;
  data.charge_current = current;
  return true;
}

bool ModbeeMpptConfig::setTerminationCurrent(float current) {
  if (current < 0.04f || current > 1.0f) return false;
  data.termination_current = current;
  return true;
}

bool ModbeeMpptConfig::setRechargeThreshold(float threshold) {
  if (threshold < 0.05f || threshold > 0.8f) return false;
  data.recharge_threshold = threshold;
  return true;
}

bool ModbeeMpptConfig::setPrechargeCurrent(float current) {
  if (current < 0.04f || current > 2.0f) return false;
  data.precharge_current = current;
  return true;
}

bool ModbeeMpptConfig::setPrechargeVoltageThreshold(modbee_vbat_lowv_t threshold) {
  data.precharge_voltage_threshold = threshold;
  return true;
}

bool ModbeeMpptConfig::setInputLimits(float voltage_limit, float current_limit) {
  if (voltage_limit < 3.6f || voltage_limit > 22.0f) return false;
  if (current_limit < 0.1f || current_limit > 3.25f) return false;
  data.input_voltage_limit = voltage_limit;
  data.input_current_limit = current_limit;
  return true;
}

bool ModbeeMpptConfig::setVACOVP(float threshold) {
  if (threshold < 6.0f || threshold > 26.0f) return false;
  data.vac_ovp_threshold = threshold;
  return true;
}

bool ModbeeMpptConfig::setTimerConfig(bool fast_enable, modbee_charge_timer_t fast_timer,
                                      bool precharge_enable, modbee_precharge_timer_t precharge_timer,
                                      modbee_topoff_timer_t topoff_timer) {
  data.fast_charge_timer_enable = fast_enable;
  data.fast_charge_timer = fast_timer;
  data.precharge_timer_enable = precharge_enable;
  data.precharge_timer = precharge_timer;
  data.topoff_timer = topoff_timer;
  return true;
}

bool ModbeeMpptConfig::setLoopIntervals(unsigned long battery_check, unsigned long soc_check) {
  if (battery_check < 1000 || battery_check > 300000) return false;
  if (soc_check < 5000 || soc_check > 600000) return false;
  
  data.battery_check_interval = battery_check;
  data.soc_check_interval = soc_check;
  return true;
}

bool ModbeeMpptConfig::setMPPTConfig(modbee_voc_percent_t voc_percent, modbee_voc_delay_t voc_delay,
                                     modbee_voc_rate_t voc_rate, bool enable) {
  data.mppt_voc_percent = voc_percent;
  data.mppt_voc_delay = voc_delay;
  data.mppt_voc_rate = voc_rate;
  data.mppt_enable = enable;
  return true;
}

// Quick individual setters that update, save, and apply to hardware immediately
bool ModbeeMpptConfig::updateChargeVoltage(float voltage) {
  if (voltage < 8.0f || voltage > 22.0f) return false;
  data.charge_voltage = voltage;
  return saveConfig();
  // Note: Hardware application should be done by caller using applySingleChange()
}

bool ModbeeMpptConfig::updateChargeCurrent(float current) {
  if (current < 0.1f || current > 5.0f) return false;
  data.charge_current = current;
  return saveConfig();
}

bool ModbeeMpptConfig::updateTerminationCurrent(float current) {
  if (current < 0.05f || current > 1.0f) return false;
  data.termination_current = current;
  return saveConfig();
}

bool ModbeeMpptConfig::updateInputCurrentLimit(float current) {
  if (current < 0.5f || current > 5.0f) return false;
  data.input_current_limit = current;
  return saveConfig();
}

bool ModbeeMpptConfig::updateBatteryCheckInterval(unsigned long interval) {
  if (interval < 1000 || interval > 300000) return false;
  data.battery_check_interval = interval;
  return saveConfig();
}

bool ModbeeMpptConfig::updateSOCCheckInterval(unsigned long interval) {
  if (interval < 5000 || interval > 600000) return false;
  data.soc_check_interval = interval;
  return saveConfig();
}

bool ModbeeMpptConfig::applySingleChange(ModbeeMpptAPI& api, const String& parameter) {
  // Apply individual parameter changes to MPPT hardware
  if (parameter == "charge_voltage") {
    return api.setChargeVoltage(data.charge_voltage);
  } else if (parameter == "charge_current") {
    return api.setChargeCurrent(data.charge_current);
  } else if (parameter == "termination_current") {
    return api.setTerminationCurrent(data.termination_current);
  } else if (parameter == "input_current_limit") {
    return api.setInputCurrentLimit(data.input_current_limit);
  } else if (parameter == "recharge_threshold") {
    return api.setRechargeThreshold(data.recharge_threshold);
  } else if (parameter == "input_voltage_limit") {
    return api.setInputVoltageLimit(data.input_voltage_limit);
  } else if (parameter == "mppt_voc_percent") {
    return api.setMPPTVOCPercent(data.mppt_voc_percent);
  } else if (parameter == "mppt_enable") {
    return api.setMPPTEnable(data.mppt_enable);
  }
  // Add more parameters as needed
  return false; // Unknown parameter
}
