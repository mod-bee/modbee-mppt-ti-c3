/*!
 * @file ModbeeMpptConfig.h
 * 
 * @brief Configuration management for ModbeeMPPT using JSON storage
 * 
 * This file handles loading, saving, and managing all user-configurable 
 * MPPT parameters using ArduinoJson for persistent storage.
 */

#ifndef MODBEE_MPPT_CONFIG_H
#define MODBEE_MPPT_CONFIG_H

#include "ModbeeMpptGlobal.h"
#include "ModbeeMpptAPI.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

// Configuration file path
#define MODBEE_CONFIG_FILE "/config/mppt_config.json"

// Configuration structure for all user-adjustable parameters
struct ModbeeMpptConfigData {
  // Battery Configuration
  modbee_battery_type_t battery_type;
  uint8_t battery_cell_count;
  float charge_voltage;
  float charge_current;
  float min_system_voltage;
  
  // Charging Control
  float termination_current;
  float recharge_threshold;
  float precharge_current;
  modbee_vbat_lowv_t precharge_voltage_threshold;
  
  // Input Limits & Protection
  float input_voltage_limit;
  float input_current_limit;
  float vac_ovp_threshold;
  
  // Timer Configuration
  bool fast_charge_timer_enable;
  modbee_charge_timer_t fast_charge_timer;
  bool precharge_timer_enable;
  modbee_precharge_timer_t precharge_timer;
  modbee_topoff_timer_t topoff_timer;
  
  // MPPT Configuration
  modbee_voc_percent_t mppt_voc_percent;
  modbee_voc_delay_t mppt_voc_delay;
  modbee_voc_rate_t mppt_voc_rate;
  bool mppt_enable;
  
  // Power Management & Noise Control
  bool pfm_forward_enable;      // PFM mode for efficiency (may cause noise at light loads)
  bool ooa_forward_enable;      // Out of Audio mode (maintain 25kHz minimum frequency)
  
  // Loop Intervals (in milliseconds)
  unsigned long battery_check_interval;
  unsigned long soc_check_interval;
  unsigned long config_apply_interval; // Interval for periodic config re-application
};

class ModbeeMpptConfig {
public:
  ModbeeMpptConfig();
  
  // Configuration management
  bool begin();
  bool loadConfig();
  bool saveConfig();
  bool resetToDefaults();
  
  // Direct access to config data
  ModbeeMpptConfigData data;  // Public direct access - simple!
  
  // Apply configuration to MPPT API
  bool applyToMPPT(class ModbeeMpptAPI& api);
  
  // Apply single configuration change to MPPT hardware
  bool applySingleChange(class ModbeeMpptAPI& api, const String& parameter);
  
  // Individual parameter setters with validation
  bool setBatteryType(modbee_battery_type_t type, uint8_t cell_count);
  bool setChargeVoltage(float voltage);
  bool setChargeCurrent(float current);
  bool setTerminationCurrent(float current);
  bool setRechargeThreshold(float threshold);
  bool setPrechargeCurrent(float current);
  bool setPrechargeVoltageThreshold(modbee_vbat_lowv_t threshold);
  bool setInputLimits(float voltage_limit, float current_limit);
  bool setVACOVP(float threshold);
  bool setTimerConfig(bool fast_enable, modbee_charge_timer_t fast_timer,
                      bool precharge_enable, modbee_precharge_timer_t precharge_timer,
                      modbee_topoff_timer_t topoff_timer);
  bool setMPPTConfig(modbee_voc_percent_t voc_percent, modbee_voc_delay_t voc_delay,
                     modbee_voc_rate_t voc_rate, bool enable);
  bool setLoopIntervals(unsigned long battery_check, unsigned long soc_check);
  
  // Quick individual setters that update and save immediately
  bool updateChargeVoltage(float voltage);
  bool updateChargeCurrent(float current);
  bool updateTerminationCurrent(float current);
  bool updateInputCurrentLimit(float current);
  bool updateBatteryCheckInterval(unsigned long interval);
  bool updateSOCCheckInterval(unsigned long interval);
  bool updateConfigApplyInterval(unsigned long interval);
  
  // Configuration validation
  bool validateConfig() const;
  
  // Debug and status
  void printConfig() const;
  String getConfigAsString() const;
  
private:
  bool _initialized;
  
  // Internal helpers
  void setDefaults();
  bool loadFromJson(const JsonDocument& doc);
  bool saveToJson(JsonDocument& doc) const;
  bool ensureConfigDirectory();
  
  // Validation helpers
  bool validateBatteryConfig() const;
  bool validateChargingConfig() const;
  bool validateInputConfig() const;
  bool validateTimerConfig() const;
  bool validateIntervalConfig() const;
};

#endif // MODBEE_MPPT_CONFIG_H
