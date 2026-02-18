/*!
 * @file ModbeeMpptAPI.cpp
 * 
 * @brief Implementation of the high-level MPPT API helper
 */

#include "ModbeeMpptGlobal.h"
#include "ModbeeMPPT.h"
#include "ModbeeMpptAPI.h"

ModbeeMpptAPI::ModbeeMpptAPI(ModbeeMPPT& mppt) : 
  _mppt(mppt),
  _battery_min_voltage(10.0f),   // Default for 4S LiFePO4
  _battery_max_voltage(14.6f),
  _battery_nominal_voltage(12.8f),
  _battery_type(MODBEE_BATTERY_LIFEPO4),
  _battery_cell_count(4),
  _tbv_state(TBVS_IDLE),
  _tbv_timer(0),
  _tbv_original_charge_state(false),
  _tbv_original_discharge_state(false),
  _tbv_last_reading(0.0f),
  _tbv_reading_valid(false)
{
  // Initialize peak power and total energy tracking variables
  _vin1PeakPower = 0.0f;
  _vin1TotalEnergyWh = 0.0f;
  _vin2PeakPower = 0.0f;
  _vin2TotalEnergyWh = 0.0f;
  _vbusPeakPower = 0.0f;
  _vbusTotalEnergyWh = 0.0f;
  _batteryPeakPower = 0.0f;
  _batteryTotalEnergyWh = 0.0f;
  _systemPeakPower = 0.0f;
  _systemTotalEnergyWh = 0.0f;
  _lastStatsUpdateMs = millis();
}

// ========================================================================
// PEAK POWER AND TOTAL ENERGY TRACKING
// ========================================================================

void ModbeeMpptAPI::updateStats() {
  unsigned long now = millis();
  float dt_hours = (now - _lastStatsUpdateMs) / 3600000.0f; // ms to hours
  _lastStatsUpdateMs = now;

  // VIN1
  float vin1_power = getVAC1Power().power;
  if (vin1_power > _vin1PeakPower) _vin1PeakPower = vin1_power;
  _vin1TotalEnergyWh += vin1_power * dt_hours;

  // VIN2
  float vin2_power = getVAC2Power().power;
  if (vin2_power > _vin2PeakPower) _vin2PeakPower = vin2_power;
  _vin2TotalEnergyWh += vin2_power * dt_hours;

  // VBUS
  float vbus_power = getVbusPower().power;
  if (vbus_power > _vbusPeakPower) _vbusPeakPower = vbus_power;
  _vbusTotalEnergyWh += vbus_power * dt_hours;

  // BAT
  float bat_power = getBatteryPower().power;
  float bat_current = getBatteryPower().current;
  if (bat_power > _batteryPeakPower) _batteryPeakPower = bat_power;
  // Only accumulate charge energy when charging
  if (bat_current > 0.0f) {
    _batteryTotalEnergyWh += bat_power * dt_hours;
  }
  // Only accumulate discharge energy when discharging
  if (bat_current < 0.0f) {
    float abs_power = -bat_power;
    _batteryWattHoursDischarge += abs_power * dt_hours;
  }

  // SYS (VSYS) - debounce peak power
  float sys_power = getSystemPower().power;
  static int sysPeakDebounce = 0;
  if (sys_power > _systemPeakPower) {
    sysPeakDebounce++;
    if (sysPeakDebounce >= 3) { // Require 3 consecutive samples above previous peak
      _systemPeakPower = sys_power;
      sysPeakDebounce = 0;
    }
  } else {
    sysPeakDebounce = 0;
  }
  _systemTotalEnergyWh += sys_power * dt_hours;

  // Battery charge/discharge amp tracking
  if (bat_current > 0.0f) {
    // Charging
    if (bat_current > _batteryPeakChargeAmps) _batteryPeakChargeAmps = bat_current;
    _batteryAmpHoursCharge += bat_current * dt_hours;
  } else if (bat_current < 0.0f) {
    // Discharging
    float abs_current = -bat_current;
    if (abs_current > _batteryPeakDischargeAmps) _batteryPeakDischargeAmps = abs_current;
    _batteryAmpHoursDischarge += abs_current * dt_hours;
  }
  // Battery discharge power tracking
  float bat_power_discharge = getBatteryPower().power;
  float bat_current_discharge = getBatteryPower().current;
  if (bat_current_discharge < 0.0f) {
    float abs_power = -bat_power_discharge;
    if (abs_power > _batteryPeakDischargePower) _batteryPeakDischargePower = abs_power;
    // _batteryWattHoursDischarge already accumulated above
  }
}

// Getters
float ModbeeMpptAPI::getVin1PeakPower() const { return _vin1PeakPower; }
float ModbeeMpptAPI::getVin1TotalEnergyWh() const { return _vin1TotalEnergyWh; }
void ModbeeMpptAPI::resetVin1Stats() {
  _vin1PeakPower = 0.0f;
  _vin1TotalEnergyWh = 0.0f;
  _mppt.statsLog.saveStatsFromAPI();
}

float ModbeeMpptAPI::getVin2PeakPower() const { return _vin2PeakPower; }
float ModbeeMpptAPI::getVin2TotalEnergyWh() const { return _vin2TotalEnergyWh; }
void ModbeeMpptAPI::resetVin2Stats() {
  _vin2PeakPower = 0.0f;
  _vin2TotalEnergyWh = 0.0f;
  _mppt.statsLog.saveStatsFromAPI();
}

float ModbeeMpptAPI::getVbusPeakPower() const { return _vbusPeakPower; }
float ModbeeMpptAPI::getVbusTotalEnergyWh() const { return _vbusTotalEnergyWh; }
void ModbeeMpptAPI::resetVbusStats() {
  _vbusPeakPower = 0.0f;
  _vbusTotalEnergyWh = 0.0f;
  _mppt.statsLog.saveStatsFromAPI();
}

float ModbeeMpptAPI::getBatteryPeakPower() const { return _batteryPeakPower; }
float ModbeeMpptAPI::getBatteryTotalEnergyWh() const { return _batteryTotalEnergyWh; }
void ModbeeMpptAPI::resetBatteryStats() {
  _batteryPeakPower = 0.0f;
  _batteryTotalEnergyWh = 0.0f;
  _mppt.statsLog.saveStatsFromAPI();
}

float ModbeeMpptAPI::getSystemPeakPower() const { return _systemPeakPower; }
float ModbeeMpptAPI::getSystemTotalEnergyWh() const { return _systemTotalEnergyWh; }
void ModbeeMpptAPI::resetSystemStats() {
  _systemPeakPower = 0.0f;
  _systemTotalEnergyWh = 0.0f;
  _mppt.statsLog.saveStatsFromAPI();
}
// ========================================================================
// Stats restoration setters
// ========================================================================
void ModbeeMpptAPI::setVin1PeakPower(float p) { _vin1PeakPower = p; }
void ModbeeMpptAPI::setVin1TotalEnergyWh(float e) { _vin1TotalEnergyWh = e; }
void ModbeeMpptAPI::setVin2PeakPower(float p) { _vin2PeakPower = p; }
void ModbeeMpptAPI::setVin2TotalEnergyWh(float e) { _vin2TotalEnergyWh = e; }
void ModbeeMpptAPI::setVbusPeakPower(float p) { _vbusPeakPower = p; }
void ModbeeMpptAPI::setVbusTotalEnergyWh(float e) { _vbusTotalEnergyWh = e; }
void ModbeeMpptAPI::setBatteryPeakPower(float p) { _batteryPeakPower = p; }
void ModbeeMpptAPI::setBatteryTotalEnergyWh(float e) { _batteryTotalEnergyWh = e; }
void ModbeeMpptAPI::setSystemPeakPower(float p) { _systemPeakPower = p; }
void ModbeeMpptAPI::setSystemTotalEnergyWh(float e) { _systemTotalEnergyWh = e; }
void ModbeeMpptAPI::setBatteryPeakChargeAmps(float a) { _batteryPeakChargeAmps = a; }
void ModbeeMpptAPI::setBatteryPeakDischargeAmps(float a) { _batteryPeakDischargeAmps = a; }
void ModbeeMpptAPI::setBatteryAmpHoursCharge(float ah) { _batteryAmpHoursCharge = ah; }
void ModbeeMpptAPI::setBatteryAmpHoursDischarge(float ah) { _batteryAmpHoursDischarge = ah; }
void ModbeeMpptAPI::setBatteryPeakDischargePower(float p) { _batteryPeakDischargePower = p; }
void ModbeeMpptAPI::setBatteryWattHoursDischarge(float wh) { _batteryWattHoursDischarge = wh; }

float ModbeeMpptAPI::getBatteryPeakChargeAmps() const { return _batteryPeakChargeAmps; }
float ModbeeMpptAPI::getBatteryPeakDischargeAmps() const { return _batteryPeakDischargeAmps; }
float ModbeeMpptAPI::getBatteryAmpHoursCharge() const { return _batteryAmpHoursCharge; }
float ModbeeMpptAPI::getBatteryAmpHoursDischarge() const { return _batteryAmpHoursDischarge; }
void ModbeeMpptAPI::resetBatteryAmpStats() {
  _batteryPeakChargeAmps = 0.0f;
  _batteryPeakDischargeAmps = 0.0f;
  _batteryAmpHoursCharge = 0.0f;
  _batteryAmpHoursDischarge = 0.0f;
  _mppt.statsLog.saveStatsFromAPI();
}

float ModbeeMpptAPI::getBatteryPeakDischargePower() const { return _batteryPeakDischargePower; }
float ModbeeMpptAPI::getBatteryWattHoursDischarge() const { return _batteryWattHoursDischarge; }
void ModbeeMpptAPI::resetBatteryDischargePowerStats() {
  _batteryPeakDischargePower = 0.0f;
  _batteryWattHoursDischarge = 0.0f;
  _mppt.statsLog.saveStatsFromAPI();
}

modbee_power_data_t ModbeeMpptAPI::getVbusPower() {
  modbee_power_data_t data = {0.0f, 0.0f, 0.0f, false};
  
  data.voltage = _mppt._bq25798.getADCVBUS();
  data.current = _mppt._bq25798.getADCIBUS();
  data.power = data.voltage * data.current;
  data.valid = (data.voltage > 0.1f); // Valid if voltage > 100mV
  
  return data;
}

modbee_power_data_t ModbeeMpptAPI::getBatteryPower() {
  modbee_power_data_t data = {0.0f, 0.0f, 0.0f, false};
  
  data.voltage = _mppt._bq25798.getADCVBAT();
  data.current = _mppt._bq25798.getADCIBAT(); // Positive = charging
  data.power = data.voltage * data.current;
  data.valid = (data.voltage > 0.1f);
  
  return data;
}

modbee_power_data_t ModbeeMpptAPI::getSystemPower() {
  modbee_power_data_t data = {0.0f, 0.0f, 0.0f, false};
  
  data.voltage = _mppt._bq25798.getADCVSYS();
  data.valid = (data.voltage > 0.1f);
  
  if (!data.valid) return data;
  
  // Calculate system current using the fundamental power balance equation:
  // IBUS * VBUS = IBAT * VBAT + ISYS * VSYS + Converter_Losses
  // Rearranging: ISYS = (IBUS * VBUS - IBAT * VBAT) / VSYS
  
  float ibus_current = _mppt._bq25798.getADCIBUS();
  float vbus_voltage = _mppt._bq25798.getADCVBUS();
  float ibat_current = _mppt._bq25798.getADCIBAT();
  float vbat_voltage = _mppt._bq25798.getADCVBAT();
  
  // Power at different domains
  float input_power = ibus_current * vbus_voltage;   // Input power (VIN domain)
  float battery_power = ibat_current * vbat_voltage; // Battery power (charging = +, discharging = -)
  
  // System power = Input power - Battery power (with converter efficiency considered)
  float available_power = input_power - battery_power;
  
  // Calculate system current from available power and system voltage
  data.current = available_power / data.voltage;
  data.power = data.current * data.voltage;  // Actual system power at VSYS voltage
  
  // Sanity checks
  if (data.current < 0.0f) {
    // If calculated current is negative, system is not being powered
    data.current = 0.0f;
    data.power = 0.0f;
  }
  
  return data;
}

modbee_power_data_t ModbeeMpptAPI::getVAC1Power() {
  modbee_power_data_t data = {0.0f, 0.0f, 0.0f, false};
  
  data.voltage = _mppt._bq25798.getADCVAC1();
  data.valid = (data.voltage > 0.1f);
  
  if (!data.valid) return data;
  
  // Calculate VAC1 current based on power conservation
  // VAC1 power should be proportional to VBUS power based on voltage ratio
  float vbus_voltage = _mppt._bq25798.getADCVBUS();
  float ibus_current = _mppt._bq25798.getADCIBUS();
  
  if (vbus_voltage > 0.1f && ibus_current > 0.001f) {
    // Assume VAC1 is the actual input source, and VBUS is just the input to the IC
    // If VAC1 ≈ VBUS, then VAC1 current ≈ IBUS current
    float voltage_ratio = data.voltage / vbus_voltage;
    data.current = ibus_current * voltage_ratio;  // Current scales inversely with voltage for same power
    data.power = data.voltage * data.current;
  } else {
    data.current = 0.0f;
    data.power = 0.0f;
  }
  
  return data;
}

modbee_power_data_t ModbeeMpptAPI::getVAC2Power() {
  modbee_power_data_t data = {0.0f, 0.0f, 0.0f, false};
  
  data.voltage = _mppt._bq25798.getADCVAC2();
  data.valid = (data.voltage > 0.1f);
  
  if (!data.valid) return data;
  
  // Calculate VAC2 current based on power conservation
  // VAC2 power should be proportional to VBUS power based on voltage ratio
  float vbus_voltage = _mppt._bq25798.getADCVBUS();
  float ibus_current = _mppt._bq25798.getADCIBUS();
  
  if (vbus_voltage > 0.1f && ibus_current > 0.001f) {
    // Assume VAC2 is the actual input source, and VBUS is just the input to the IC
    // If VAC2 ≈ VBUS, then VAC2 current ≈ IBUS current
    float voltage_ratio = data.voltage / vbus_voltage;
    data.current = ibus_current * voltage_ratio;  // Current scales inversely with voltage for same power
    data.power = data.voltage * data.current;
  } else {
    data.current = 0.0f;
    data.power = 0.0f;
  }
  
  return data;
}

float ModbeeMpptAPI::getEfficiency() {
  modbee_power_data_t input = getVbusPower();
  modbee_power_data_t battery = getBatteryPower();
  modbee_power_data_t system = getSystemPower();
  
  if (input.power <= 0.1f) return 0.0f; // Avoid division by zero
  
  // Efficiency = Total output power / Input power
  // Total output = Battery power (if charging) + System power
  float total_output_power = system.power;
  
  // Only add battery power if it's positive (charging)
  if (battery.power > 0.0f) {
    total_output_power += battery.power;
  }
  
  float efficiency = (total_output_power / input.power) * 100.0f;
  return clampValue(efficiency, 0.0f, 100.0f); // Allow slightly over 100% due to measurement errors
}

// ========================================================================
// INDIVIDUAL VOLTAGE AND CURRENT MEASUREMENTS
// ========================================================================

float ModbeeMpptAPI::getVbusVoltage() {
  return _mppt._bq25798.getADCVBUS();
}

float ModbeeMpptAPI::getIbusCurrent() {
  return _mppt._bq25798.getADCIBUS();
}

float ModbeeMpptAPI::getBatteryVoltage() {
  return _mppt._bq25798.getADCVBAT();
}

float ModbeeMpptAPI::getBatteryCurrent() {
  return _mppt._bq25798.getADCIBAT();
}

float ModbeeMpptAPI::getSystemVoltage() {
  return _mppt._bq25798.getADCVSYS();
}

float ModbeeMpptAPI::getSystemCurrent() {
  // Use the corrected system power calculation to get accurate current
  modbee_power_data_t system_power = getSystemPower();
  return system_power.current;
}

float ModbeeMpptAPI::getVAC1Voltage() {
  return _mppt._bq25798.getADCVAC1();
}

float ModbeeMpptAPI::getVAC2Voltage() {
  return _mppt._bq25798.getADCVAC2();
}

// ========================================================================
// BATTERY FUNCTIONS
// ========================================================================

float ModbeeMpptAPI::getBatteryChargePercent() {
  // Use true battery voltage for more accurate SOC calculation
  float voltage = getTrueBatteryVoltage();
  return calculateBatterySOC(voltage);
}

float ModbeeMpptAPI::getBatteryTemperature() {
  // Get TS ADC reading as percentage (0-100%)
  float ts_percent = _mppt._bq25798.getADCTS();
  
  // Convert percentage to actual temperature using 10k NTC thermistor characteristics
  // The BQ25798 uses a voltage divider with the NTC thermistor
  // TS% = (V_TS / V_REG) * 100
  // For a 10k NTC at 25°C with beta = 3380K (typical values)
  
  // Convert percentage to voltage ratio (0.0 to 1.0)
  float voltage_ratio = ts_percent / 100.0f;
  
  // Calculate thermistor resistance from voltage divider
  // Assuming pull-up resistor of 10k (typical for BQ25798)
  // R_NTC = R_PULLUP * (V_TS / (V_REG - V_TS))
  // voltage_ratio = V_TS / V_REG, so:
  // R_NTC = R_PULLUP * (voltage_ratio / (1 - voltage_ratio))
  const float R_PULLUP = 10000.0f;  // 10k pull-up resistor
  
  if (voltage_ratio >= 0.999f) {
    // Prevent division by zero, return very high temperature
    return 150.0f;
  }
  
  float r_ntc = R_PULLUP * (voltage_ratio / (1.0f - voltage_ratio));
  
  // Convert resistance to temperature using Steinhart-Hart equation (simplified beta model)
  // 1/T = 1/T0 + (1/Beta) * ln(R/R0)
  // Where: T0 = 298.15K (25°C), R0 = 10000 ohms, Beta = 3380K (typical for 10k NTC)
  const float T0 = 298.15f;     // 25°C in Kelvin
  const float R0 = 10000.0f;    // 10k reference resistance at 25°C  
  const float BETA = 3380.0f;   // Beta coefficient for typical 10k NTC
  
  if (r_ntc <= 0) {
    // Invalid resistance, return error temperature
    return -40.0f;
  }
  
  float temp_kelvin = 1.0f / ((1.0f / T0) + (1.0f / BETA) * log(r_ntc / R0));
  float temp_celsius = temp_kelvin - 273.15f;
  
  // Clamp to reasonable range (-40°C to 150°C)
  if (temp_celsius < -40.0f) temp_celsius = -40.0f;
  if (temp_celsius > 150.0f) temp_celsius = 150.0f;
  
  return temp_celsius;
}

bool ModbeeMpptAPI::setBatteryType(modbee_battery_type_t type, uint8_t cell_count) {
  if (cell_count == 0 || cell_count > 4) return false;  // BQ25798 supports max 4S
  
  // Set voltage limits based on battery type
  float cell_min, cell_max;
  
  switch (type) {
    case MODBEE_BATTERY_LIFEPO4:
      cell_min = MODBEE_LIFEPO4_MIN_VOLTAGE;
      cell_max = MODBEE_LIFEPO4_MAX_VOLTAGE;
      break;
    case MODBEE_BATTERY_LIPO:
      cell_min = MODBEE_LIPO_MIN_VOLTAGE;
      cell_max = MODBEE_LIPO_MAX_VOLTAGE;
      break;
    case MODBEE_BATTERY_LEAD_ACID:
      cell_min = MODBEE_LEAD_ACID_MIN_VOLTAGE;
      cell_max = MODBEE_LEAD_ACID_MAX_VOLTAGE;
      break;
    default:
      return false;
  }
  
  _battery_min_voltage = cell_min * cell_count;
  _battery_max_voltage = cell_max * cell_count;
  
  // Set the cell count in the BQ25798 chip
  bq25798_cell_count_t bq_cell_count;
  switch (cell_count) {
    case 1: bq_cell_count = BQ25798_CELL_COUNT_1S; break;
    case 2: bq_cell_count = BQ25798_CELL_COUNT_2S; break;
    case 3: bq_cell_count = BQ25798_CELL_COUNT_3S; break;
    case 4: bq_cell_count = BQ25798_CELL_COUNT_4S; break;
    default: return false;

  }
  return _mppt._bq25798.setCellCount(bq_cell_count);
}

/*!
 * @brief Get true battery voltage by temporarily stopping charging and forcing discharge
 * 
 * This function provides the actual battery open-circuit voltage by:
 * 1. Temporarily disabling charging
 * 2. Forcing a small discharge current
 * 3. Reading the true battery voltage
 * 4. Restoring original charging state
 * 
 * This voltage represents the actual battery state without charging influence.
 * 
 * @return True battery voltage in volts
 */
/*!
 * @brief Update true battery voltage measurement (start measurement process)
 * 
 * This function initiates the true battery voltage measurement process if one
 * is not already in progress. The measurement temporarily disables charging 
 * and forces a small discharge current to measure the true no-load battery voltage.
 */
void ModbeeMpptAPI::updateTrueBatteryVoltage() {
  // Start a new measurement if not currently in progress
  if (_tbv_state == TBVS_IDLE) {
    _tbv_reading_valid = false; // Invalidate old reading to get fresh measurement
    _tbv_state = TBVS_DISABLE_CHARGING;
    _tbv_timer = millis();
  }
}

/*!
 * @brief Get the last measured true battery voltage
 * 
 * When charging: Returns the most recent true battery voltage reading from the state machine.
 * When not charging: Returns the current ADC battery voltage (no need for complex measurement).
 * 
 * @return True battery voltage in volts
 */
float ModbeeMpptAPI::getTrueBatteryVoltage() {
  // If charging, use the last true battery voltage measurement from state machine
  if (isCharging()) {
    return _tbv_last_reading;
  } else {
    // If not charging, the current VBAT reading is effectively the true battery voltage
    return _mppt._bq25798.getADCVBAT();
  }
}

void ModbeeMpptAPI::update() {
  // Update true battery voltage state machine
  unsigned long currentTime = millis();
  
  switch (_tbv_state) {
    case TBVS_IDLE:
      // Nothing to do
      break;
      
    case TBVS_DISABLE_CHARGING:
      // Store original states and disable charging
      //_tbv_original_charge_state = getChargeEnable();
      _tbv_original_discharge_state = _mppt._bq25798.getForceBattDischarge();
      //setChargeEnable(false);
      _tbv_state = TBVS_WAIT_CHARGE_STOP;
      _tbv_timer = currentTime;
      break;
      
    case TBVS_WAIT_CHARGE_STOP:
      if (currentTime - _tbv_timer >= 50) { // Wait 50ms
        _tbv_state = TBVS_FORCE_DISCHARGE;
        _tbv_timer = currentTime;
      }
      break;
      
    case TBVS_FORCE_DISCHARGE:
      // Force small discharge current
      _mppt._bq25798.setForceBattDischarge(true);
      _tbv_state = TBVS_WAIT_STABILIZE;
      _tbv_timer = currentTime;
      break;
      
    case TBVS_WAIT_STABILIZE:
      if (currentTime - _tbv_timer >= 400) { // Wait 400ms
        _tbv_state = TBVS_READ_VOLTAGE;
      }
      break;
      
    case TBVS_READ_VOLTAGE:
      // Read the true battery voltage
      _tbv_last_reading = _mppt._bq25798.getADCVBAT();
      _tbv_reading_valid = true;
      _tbv_state = TBVS_WAIT_BEFORE_RESTORE;
      _tbv_timer = currentTime;
      break;
      
    case TBVS_WAIT_BEFORE_RESTORE:
      if (currentTime - _tbv_timer >= 50) { // Wait 50ms before restoring charging
        _tbv_state = TBVS_RESTORE;
      }
      break;
      
    case TBVS_RESTORE:
      // Restore original states
      _mppt._bq25798.setForceBattDischarge(_tbv_original_discharge_state);
      //setChargeEnable(_tbv_original_charge_state);
      _tbv_state = TBVS_IDLE;
      break;
  }
}

/*!
 * @brief Get battery voltage while charging (charging terminal voltage)
 * 
 * This is the voltage measured at the battery terminals while charging is active.
 * This voltage will be higher than the true battery voltage due to charging current.
 * 
 * @return Battery charging voltage in volts
 */
float ModbeeMpptAPI::getBatteryChargingVoltage() {
  return _mppt._bq25798.getADCVBAT();
}

/*!
 * @brief Get actual battery state of charge based on full battery voltage range
 * 
 * This SOC is calculated using the true battery voltage and the full voltage range
 * of the configured battery chemistry (e.g., 2.5V to 4.2V for Li-Ion per cell).
 * This represents the absolute battery charge state.
 * 
 * @return Actual battery SOC percentage (0.0 - 100.0)
 */
float ModbeeMpptAPI::getActualBatterySOC() {
  float true_voltage = getTrueBatteryVoltage();
  return calculateBatterySOC(true_voltage);
}

/*!
 * @brief Get usable battery state of charge based on system operating range
 * 
 * This SOC is calculated using the true battery voltage but limited to the
 * system's usable voltage range (min system voltage to charge voltage).
 * This represents the practically usable energy in the system.
 * 
 * @return Usable battery SOC percentage (0.0 - 100.0)
 */
float ModbeeMpptAPI::getUsableBatterySOC() {
  float true_voltage = getTrueBatteryVoltage();
  float min_system_voltage = _mppt._bq25798.getMinSystemV();
  float charge_voltage = _mppt._bq25798.getChargeLimitV();
  
  // Calculate SOC based on usable voltage range
  if (true_voltage < min_system_voltage) return 0.0f;
  if (true_voltage > charge_voltage) return 100.0f;
  
  float usable_range = charge_voltage - min_system_voltage;
  float usable_position = true_voltage - min_system_voltage;
  
  return (usable_position / usable_range) * 100.0f;
}

/*!
 * @brief Get comprehensive battery status with all voltage and SOC readings
 * 
 * This function provides a complete battery status including:
 * - Charging voltage (voltage while charging is active)
 * - True battery voltage (voltage with charging temporarily stopped)
 * - Actual SOC (based on full battery chemistry range)
 * - Usable SOC (based on system operating range)
 * 
 * @return Structure containing all battery voltage and SOC readings
 */
modbee_battery_status_t ModbeeMpptAPI::getComprehensiveBatteryStatus() {
  modbee_battery_status_t status;
  
  status.charging_voltage = getBatteryChargingVoltage();
  status.true_voltage = getTrueBatteryVoltage();
  status.actual_soc = getActualBatterySOC();
  status.usable_soc = getUsableBatterySOC();
  status.current = getBatteryCurrent();
  status.temperature = getBatteryTemperature();
  
  // Determine charging state
  if (status.current > 0.01f) {
    status.state = "Charging";
  } else if (status.current < -0.01f) {
    status.state = "Discharging";
  } else {
    status.state = "Idle";
  }
  
  return status;
}

// ========================================================================
// CHARGING CONTROL FUNCTIONS (with safety clamping)
// ========================================================================

bool ModbeeMpptAPI::setChargeVoltage(float voltage) {
  float clamped = clampValue(voltage, MODBEE_MIN_CHARGE_VOLTAGE, MODBEE_MAX_CHARGE_VOLTAGE);
  return _mppt._bq25798.setChargeLimitV(clamped);
}

bool ModbeeMpptAPI::setChargeCurrent(float current) {
  float clamped = clampValue(current, MODBEE_MIN_CHARGE_CURRENT, MODBEE_MAX_CHARGE_CURRENT);
  return _mppt._bq25798.setChargeLimitA(clamped);
}

float ModbeeMpptAPI::getTerminationCurrent() {
  return _mppt._bq25798.getTerminationA();
}

bool ModbeeMpptAPI::setTerminationCurrent(float current) {
  // BQ25798 ITERM range: 40mA to 1000mA (0.04A to 1.0A)
  float clamped = clampValue(current, 0.04f, 1.0f);
  return _mppt._bq25798.setTerminationA(clamped);
}

float ModbeeMpptAPI::getRechargeThreshold() {
  return _mppt._bq25798.getRechargeThreshOffsetV();
}

bool ModbeeMpptAPI::setRechargeThreshold(float offset) {
  // BQ25798 VRECHG range: 50mV to 800mV (0.05V to 0.8V)
  float clamped = clampValue(offset, 0.05f, 0.8f);
  return _mppt._bq25798.setRechargeThreshOffsetV(clamped);
}

float ModbeeMpptAPI::getPrechargeCurrent() {
  return _mppt._bq25798.getPrechargeLimitA();
}

bool ModbeeMpptAPI::setPrechargeCurrent(float current) {
  // BQ25798 IPRECHG range: 40mA to 2000mA (0.04A to 2.0A)
  float clamped = clampValue(current, 0.04f, 2.0f);
  return _mppt._bq25798.setPrechargeLimitA(clamped);
}

modbee_vbat_lowv_t ModbeeMpptAPI::getPrechargeVoltageThreshold() {
  return (modbee_vbat_lowv_t)_mppt._bq25798.getVBatLowV();
}

bool ModbeeMpptAPI::setPrechargeVoltageThreshold(modbee_vbat_lowv_t threshold) {
  return _mppt._bq25798.setVBatLowV((bq25798_vbat_lowv_t)threshold);
}

bool ModbeeMpptAPI::setInputCurrentLimit(float current) {
  float clamped = clampValue(current, 0.1f, 3.3f); // BQ25798 input current range: 100mA-3300mA
  return _mppt._bq25798.setInputLimitA(clamped);
}

bool ModbeeMpptAPI::setMinSystemVoltage(float voltage) {
  float clamped = clampValue(voltage, 2.5f, 16.0f); // BQ25798 VSYSMIN range: 2500mV-16000mV
  return _mppt._bq25798.setMinSystemV(clamped);
}


// ========================================================================
// INPUT VOLTAGE AND CURRENT LIMITS  
// ========================================================================

bool ModbeeMpptAPI::setInputVoltageLimit(float voltage) {
  float clamped = clampValue(voltage, MODBEE_MIN_INPUT_VOLTAGE, MODBEE_MAX_INPUT_VOLTAGE);
  return _mppt._bq25798.setInputLimitV(clamped);
}

float ModbeeMpptAPI::getInputVoltageLimit() {
  return _mppt._bq25798.getInputLimitV();
}

float ModbeeMpptAPI::getMinSystemVoltage() {
  return _mppt._bq25798.getMinSystemV();
}

bool ModbeeMpptAPI::setVACOVP(float voltage) {
  // Map voltage to BQ25798 VAC OVP settings
  bq25798_vac_ovp_t ovp_setting;
  if (voltage <= 7.0f) {
    ovp_setting = BQ25798_VAC_OVP_7V;
  } else if (voltage <= 12.0f) {
    ovp_setting = BQ25798_VAC_OVP_12V;
  } else if (voltage <= 22.0f) {
    ovp_setting = BQ25798_VAC_OVP_22V;
  } else {
    ovp_setting = BQ25798_VAC_OVP_26V;
  }
  return _mppt._bq25798.setVACOVP(ovp_setting);
}

float ModbeeMpptAPI::getVACOVP() {
  bq25798_vac_ovp_t ovp_setting = _mppt._bq25798.getVACOVP();
  switch (ovp_setting) {
    case BQ25798_VAC_OVP_7V:  return 7.0f;
    case BQ25798_VAC_OVP_12V: return 12.0f;
    case BQ25798_VAC_OVP_22V: return 22.0f;
    case BQ25798_VAC_OVP_26V: return 26.0f;
    default: return 7.0f;
  }
}

uint8_t ModbeeMpptAPI::getCellCount() {
  bq25798_cell_count_t bq_count = _mppt._bq25798.getCellCount();
  switch (bq_count) {
    case BQ25798_CELL_COUNT_1S: return 1;
    case BQ25798_CELL_COUNT_2S: return 2;
    case BQ25798_CELL_COUNT_3S: return 3;
    case BQ25798_CELL_COUNT_4S: return 4;
    default: return 1;
  }
}

// ========================================================================
// ADC CONTROL FUNCTIONS
// ========================================================================

bool ModbeeMpptAPI::setADCEnable(bool enable) {
  return _mppt._bq25798.setADCEnable(enable);
}

bool ModbeeMpptAPI::getADCEnable() {
  return _mppt._bq25798.getADCEnable();
}

bool ModbeeMpptAPI::setADCMode(modbee_adc_mode_t mode) {
  bq25798_adc_rate_t bq_mode = (mode == MODBEE_ADC_CONTINUOUS) ? 
    BQ25798_ADC_RATE_CONTINUOUS : BQ25798_ADC_RATE_ONE_SHOT;
  return _mppt._bq25798.setADCRate(bq_mode);
}

bool ModbeeMpptAPI::setADCAveraging(modbee_adc_avg_t averaging) {
  bq25798_adc_avg_t bq_avg = static_cast<bq25798_adc_avg_t>(averaging);
  return _mppt._bq25798.setADCAverage(bq_avg);
}

bool ModbeeMpptAPI::setADCResolution(modbee_adc_res_t resolution) {
  bq25798_adc_res_t bq_res = static_cast<bq25798_adc_res_t>(resolution);
  return _mppt._bq25798.setADCResolution(bq_res);
}

bool ModbeeMpptAPI::configureADC(modbee_adc_res_t resolution, 
                                 modbee_adc_avg_t averaging, 
                                 modbee_adc_mode_t mode) {
  // Convert Modbee types to BQ25798 types
  bq25798_adc_res_t bq_res = static_cast<bq25798_adc_res_t>(resolution);
  bq25798_adc_avg_t bq_avg = static_cast<bq25798_adc_avg_t>(averaging);
  bq25798_adc_rate_t bq_rate = (mode == MODBEE_ADC_CONTINUOUS) ? 
    BQ25798_ADC_RATE_CONTINUOUS : BQ25798_ADC_RATE_ONE_SHOT;
  
  // Use the BQ25798 library's combined configuration function
  return _mppt._bq25798.configureADC(bq_res, bq_avg, bq_rate);
}

bool ModbeeMpptAPI::isADCConversionDone() {
  return _mppt._bq25798.isADCConversionDone();
}

/*!
 * @brief Debug function to read raw ADC control register
 * @return Raw ADC control register value
 */
uint8_t ModbeeMpptAPI::getADCControlRegister() {
  uint8_t value;
  // Direct register read using public wrapper
  _mppt._bq25798.readRegisterDirect(0x2E, &value); // BQ25798_REG_ADC_CONTROL
  return value;
}

// ========================================================================
// TIMER CONTROL FUNCTIONS
// ========================================================================

bool ModbeeMpptAPI::setFastChargeTimer(modbee_charge_timer_t timer) {
  return _mppt._bq25798.setFastChargeTimer((bq25798_chg_timer_t)timer);
}

bool ModbeeMpptAPI::setFastChargeTimerEnable(bool enable) {
  return _mppt._bq25798.setFastChargeTimerEnable(enable);
}

bool ModbeeMpptAPI::getFastChargeTimerEnable() {
  return _mppt._bq25798.getFastChargeTimerEnable();
}

bool ModbeeMpptAPI::setPrechargeTimer(modbee_precharge_timer_t timer) {
  return _mppt._bq25798.setPrechargeTimer((bq25798_prechg_timer_t)timer);
}

bool ModbeeMpptAPI::setPrechargeTimerEnable(bool enable) {
  return _mppt._bq25798.setPrechargeTimerEnable(enable);
}

bool ModbeeMpptAPI::getPrechargeTimerEnable() {
  return _mppt._bq25798.getPrechargeTimerEnable();
}

modbee_topoff_timer_t ModbeeMpptAPI::getTopOffTimer() {
  return (modbee_topoff_timer_t)_mppt._bq25798.getTopOffTimer();
}

bool ModbeeMpptAPI::setTopOffTimer(modbee_topoff_timer_t timer) {
  return _mppt._bq25798.setTopOffTimer((bq25798_topoff_timer_t)timer);
}

bool ModbeeMpptAPI::setTrickleChargeTimerEnable(bool enable) {
  return _mppt._bq25798.setTrickleChargeTimerEnable(enable);
}

bool ModbeeMpptAPI::getTrickleChargeTimerEnable() {
  return _mppt._bq25798.getTrickleChargeTimerEnable();
}

bool ModbeeMpptAPI::setTimerHalfRateEnable(bool enable) {
  return _mppt._bq25798.setTimerHalfRateEnable(enable);
}

bool ModbeeMpptAPI::getTimerHalfRateEnable() {
  return _mppt._bq25798.getTimerHalfRateEnable();
}

// ========================================================================
// MPPT CONTROL FUNCTIONS
// ========================================================================

bool ModbeeMpptAPI::setMPPTEnable(bool enable) {
  return _mppt._bq25798.setMPPTenable(enable);
}

bool ModbeeMpptAPI::getMPPTEnable() {
  return _mppt._bq25798.getMPPTenable();
}

// ========================================================================
// SYSTEM CONTROL FUNCTIONS
// ========================================================================

bool ModbeeMpptAPI::setWatchdogEnable(bool enable) {
  if (enable) {
    return _mppt._bq25798.setWDT(BQ25798_WDT_160S);
  } else {
    return _mppt._bq25798.setWDT(BQ25798_WDT_DISABLE);
  }
}

bool ModbeeMpptAPI::setWatchdogTimer(modbee_watchdog_timer_t timer) {
  // Map our enum to BQ25798 enum and set the timer
  bq25798_wdt_t bq_timer;
  switch (timer) {
    case MODBEE_WATCHDOG_DISABLE:
      bq_timer = BQ25798_WDT_DISABLE;
      break;
    case MODBEE_WATCHDOG_40S:
      bq_timer = BQ25798_WDT_40S;
      break;
    case MODBEE_WATCHDOG_80S:
      bq_timer = BQ25798_WDT_80S;
      break;
    case MODBEE_WATCHDOG_160S:
      bq_timer = BQ25798_WDT_160S;
      break;
    default:
      return false; // Invalid timer value
  }
  
  return _mppt._bq25798.setWDT(bq_timer);
}

bool ModbeeMpptAPI::resetWatchdog() {
  return _mppt._bq25798.resetWDT();
}

bool ModbeeMpptAPI::setPWMFrequency(modbee_pwm_freq_t frequency) {
  // Convert Modbee enum to BQ25798 enum
  bq25798_pwm_freq_t bq_freq = static_cast<bq25798_pwm_freq_t>(frequency);
  return _mppt._bq25798.setPWMFrequency(bq_freq);
}

modbee_pwm_freq_t ModbeeMpptAPI::getPWMFrequency() {
  // Convert BQ25798 enum to Modbee enum
  bq25798_pwm_freq_t bq_freq = _mppt._bq25798.getPWMFrequency();
  return static_cast<modbee_pwm_freq_t>(bq_freq);
}

bool ModbeeMpptAPI::setForwardPFM(bool enable) {
  return _mppt._bq25798.setForwardPFM(enable);
}

bool ModbeeMpptAPI::getForwardPFM() {
  return _mppt._bq25798.getForwardPFM();
}

bool ModbeeMpptAPI::setForwardOOA(bool enable) {
  return _mppt._bq25798.setForwardOOA(enable);
}

bool ModbeeMpptAPI::getForwardOOA() {
  return _mppt._bq25798.getForwardOOA();
}

bool ModbeeMpptAPI::getWatchdogEnable() {
  // Watchdog is enabled if timer is not set to disable
  bq25798_wdt_t timer = _mppt._bq25798.getWDT();
  return (timer != BQ25798_WDT_DISABLE);
}

modbee_watchdog_timer_t ModbeeMpptAPI::getWatchdogTimer() {
  // Get the raw timer value from BQ25798 and convert to our enum
  bq25798_wdt_t timer = _mppt._bq25798.getWDT();
  
  // Map BQ25798 enum to our simplified enum (we only support the main timer values)
  switch(timer) {
    case BQ25798_WDT_DISABLE:
      return MODBEE_WATCHDOG_DISABLE;
    case BQ25798_WDT_40S:
      return MODBEE_WATCHDOG_40S;
    case BQ25798_WDT_80S:
      return MODBEE_WATCHDOG_80S;
    case BQ25798_WDT_160S:
      return MODBEE_WATCHDOG_160S;
    default:
      // For any other values (0.5s, 1s, 2s, 20s), return closest match
      return MODBEE_WATCHDOG_40S;  // Default
  }
}

bool ModbeeMpptAPI::setShippingMode(bool enable) {
  return _mppt._bq25798.setShipFETmode(enable ? BQ25798_SDRV_SHIP : BQ25798_SDRV_IDLE);
}

bool ModbeeMpptAPI::getShipMode() {
  bq25798_sdrv_ctrl_t mode = _mppt._bq25798.getShipFETmode();
  return (mode == BQ25798_SDRV_SHIP);
}

float ModbeeMpptAPI::getDieTemperature() {
  return _mppt._bq25798.getADCTDIE();
}

bool ModbeeMpptAPI::detectBatteryConnected() {
  // Implementation based on BQ25798 datasheet section 9.3.6
  // Method: Disable charging, force IBAT discharge, then read ADC BAT voltage
  
  // Store current states
  //bool originalChargeState = getChargeEnable();
  //bool originalDischargeState = _mppt._bq25798.getForceBattDischarge();
  
  // Step 1: Disable charging to stop BAT pin capacitance charging
  //setChargeEnable(false);
  
  // Step 2: Force battery discharge current (as per datasheet)
  _mppt._bq25798.setForceBattDischarge(true);
  
  // Step 3: Read VBAT voltage directly
  float vbat = _mppt._bq25798.getADCVBAT();
  
  // Step 4: Analyze the results
  // With no battery and discharge forced, VBAT should drop significantly below VSYS
  // With a real battery, VBAT should remain close to battery voltage
  bool batteryVoltageRange = (vbat > MODBEE_MIN_BATTERY_VOLTAGE && vbat < MODBEE_MAX_BATTERY_VOLTAGE);  // Reasonable battery range

  // Step 5: Restore original states
  _mppt._bq25798.setForceBattDischarge(false);
  //setChargeEnable(originalChargeState);
  
  // Battery is likely connected if:
  // 1. VBAT doesn't drop significantly when discharge is forced
  // 2. VBAT is in reasonable battery voltage range
  
  return batteryVoltageRange;
}

bool ModbeeMpptAPI::setBatteryDischargeSenseEnable(bool enable) {
  return _mppt._bq25798.setBatDischargeSenseEnable(enable);
}

bool ModbeeMpptAPI::getBatteryDischargeSenseEnable() {
  return _mppt._bq25798.getBatDischargeSenseEnable();
}

// ========================================================================
// ICO CONTROL FUNCTION
// ========================================================================

bool ModbeeMpptAPI::setICOEnable(bool enable) {
  // The BQ25798 ICO is controlled via the IINDPM_AUTO bit in the input current register
  // When enabled, the charger automatically runs ICO to optimize input current
  // When disabled, ICO is off and input current is set manually
  // This function disables ICO by clearing IINDPM_AUTO
  // The register interface is exposed via _mppt._bq25798.setICOEnable(bool)
  return _mppt._bq25798.setICOEnable(enable);
}

// ========================================================================
// STATUS AND FAULT FUNCTIONS
// ========================================================================

String ModbeeMpptAPI::getChargeStateString() {
  modbee_status1_t status1 = getStatus1();
  
  switch (status1.charge_state) {
    case MODBEE_CHARGE_NOT_CHARGING: return "Not Charging";
    case MODBEE_CHARGE_TRICKLE: return "Trickle Charge";
    case MODBEE_CHARGE_PRECHARGE: return "Pre-charge";
    case MODBEE_CHARGE_FAST_CC: return "Fast Charge (CC)";
    case MODBEE_CHARGE_TAPER_CV: return "Taper Charge (CV)";
    case MODBEE_CHARGE_RESERVED: return "Reserved";
    case MODBEE_CHARGE_TOPOFF: return "Top-off Timer Active";
    case MODBEE_CHARGE_DONE: return "Charge Termination Done";
    default: return "Unknown";
  }
}

bool ModbeeMpptAPI::isCharging() {
  modbee_status1_t status1 = getStatus1();
  
  // Return true for any active charging state
  switch (status1.charge_state) {
    case MODBEE_CHARGE_TRICKLE:
    case MODBEE_CHARGE_PRECHARGE:
    case MODBEE_CHARGE_FAST_CC:
    case MODBEE_CHARGE_TAPER_CV:
    case MODBEE_CHARGE_TOPOFF:
      return true;
      
    case MODBEE_CHARGE_NOT_CHARGING:
    case MODBEE_CHARGE_DONE:
    case MODBEE_CHARGE_RESERVED:
    default:
      return false;
  }
}

String ModbeeMpptAPI::getStatus0String() {
  modbee_status0_t status = getStatus0();
  
  String result = "";
  if (status.iindpm_active) result += "IINDPM ";          // Input current DPM active
  if (status.vindpm_active) result += "VINDPM ";          // Input voltage DPM active  
  if (status.watchdog_expired) result += "WD_EXPIRED ";   // Watchdog timer expired
  if (status.power_good) result += "POWER_GOOD ";         // Power good status
  if (status.ac2_present) result += "AC2_PRESENT ";       // VAC2 present
  if (status.ac1_present) result += "AC1_PRESENT ";       // VAC1 present
  if (status.vbus_present) result += "VBUS_PRESENT ";     // VBUS present
  
  return result.length() > 0 ? result.substring(0, result.length()-1) : "Normal";
}

String ModbeeMpptAPI::getStatus1String() {
  modbee_status1_t status = getStatus1();
  
  String result = "";
  
  // Charge status
  switch(status.charge_state) {
    case MODBEE_CHARGE_NOT_CHARGING: result += "Not Charging"; break;
    case MODBEE_CHARGE_TRICKLE: result += "Trickle Charge"; break;
    case MODBEE_CHARGE_PRECHARGE: result += "Pre-charge"; break;
    case MODBEE_CHARGE_FAST_CC: result += "Fast Charge (CC)"; break;
    case MODBEE_CHARGE_TAPER_CV: result += "Taper Charge (CV)"; break;
    case MODBEE_CHARGE_RESERVED: result += "Reserved"; break;
    case MODBEE_CHARGE_TOPOFF: result += "Top-off Timer Active"; break;
    case MODBEE_CHARGE_DONE: result += "Charge Termination Done"; break;
  }
  
  // Additional status bits
  if (status.bc12_done) result += " BC12_DONE";         // BC1.2 detection done
  
  return result;
}

String ModbeeMpptAPI::getStatus2String() {
  modbee_status2_t status = getStatus2();
  
  String result = "";
  
  // ICO status
  switch (status.ico_status) {
    case MODBEE_ICO_DISABLED: result += "ICO Disabled"; break;
    case MODBEE_ICO_IN_PROGRESS: result += "ICO In Progress"; break;
    case MODBEE_ICO_MAX_CURRENT: result += "ICO Max Current Detected"; break;
    case MODBEE_ICO_RESERVED: result += "ICO Reserved"; break;
  }
  
  // Additional status bits
  if (status.thermal_regulation) result += " THERMAL_REG";
  if (status.dpdm_detection_ongoing) result += " DPDM_ONGOING";
  if (status.battery_present) result += " BATTERY_PRESENT";
  
  return result;
}

String ModbeeMpptAPI::getStatus3String() {
  modbee_status3_t status = getStatus3();
  
  String result = "";
  if (status.adc_conversion_done) result += "ADC_DONE ";
  if (status.vsys_regulation) result += "VSYS_REG ";
  if (status.charge_timer_expired) result += "CHG_TMR_EXP ";
  if (status.trickle_timer_expired) result += "TRICKLE_TMR_EXP ";
  if (status.precharge_timer_expired) result += "PRECHG_TMR_EXP ";
  if (status.acrb1_active) result += "ACRB1_ACTIVE ";
  if (status.acrb2_active) result += "ACRB2_ACTIVE ";
  
  return result.length() > 0 ? result.substring(0, result.length()-1) : "Normal";
}

String ModbeeMpptAPI::getStatus4String() {
  modbee_status4_t status = getStatus4();
  
  String result = "";
  if (status.ts_hot) result += "TS_HOT ";
  if (status.ts_warm) result += "TS_WARM ";
  if (status.ts_cool) result += "TS_COOL ";
  if (status.ts_cold) result += "TS_COLD ";
  if (status.vbat_otg_low) result += "VBAT_OTG_LOW ";
  
  return result.length() > 0 ? result.substring(0, result.length()-1) : "Normal";
}

String ModbeeMpptAPI::getBatteryCurrentDirection() {
  float current = _mppt._bq25798.getADCIBAT();
  
  if (current > 0.01f) {  // Positive current = charging (threshold to avoid noise)
    return "Charging";
  } else if (current < -0.01f) {  // Negative current = discharging
    return "Discharging";
  } else {
    return "Idle";  // Very low current, essentially idle
  }
}

bool ModbeeMpptAPI::hasFaults() {
  modbee_fault0_t fault0 = getFault0();
  modbee_fault1_t fault1 = getFault1();
  
  return fault0.vac1_ovp || fault0.vac2_ovp || fault0.converter_ocp || fault0.ibat_ocp ||
         fault0.ibus_ocp || fault0.vbat_ovp || fault0.vbus_ovp || fault0.ibat_regulation ||
         fault1.thermal_shutdown || fault1.otg_uvp || fault1.otg_ovp || fault1.vsys_ovp ||
         fault1.vsys_short;
}

String ModbeeMpptAPI::getFaultString() {
  modbee_fault0_t fault0 = getFault0();
  modbee_fault1_t fault1 = getFault1();
  
  // Check if no faults
  if (!fault0.vac1_ovp && !fault0.vac2_ovp && !fault0.converter_ocp && !fault0.ibat_ocp &&
      !fault0.ibus_ocp && !fault0.vbat_ovp && !fault0.vbus_ovp && !fault0.ibat_regulation &&
      !fault1.thermal_shutdown && !fault1.otg_uvp && !fault1.otg_ovp && !fault1.vsys_ovp &&
      !fault1.vsys_short) {
    return "No faults";
  }
  
  String result = "";
  
  // Decode fault0 register (corrected to match BQ25798 datasheet)
  if (fault0.vac1_ovp) result += "VAC1 OVP; ";      // VAC1 overvoltage
  if (fault0.vac2_ovp) result += "VAC2 OVP; ";      // VAC2 overvoltage  
  if (fault0.converter_ocp) result += "CONV OCP; ";      // Converter overcurrent
  if (fault0.ibat_ocp) result += "IBAT OCP; ";      // Battery overcurrent
  if (fault0.ibus_ocp) result += "IBUS OCP; ";      // Input bus overcurrent
  if (fault0.vbat_ovp) result += "VBAT OVP; ";      // Battery overvoltage
  if (fault0.vbus_ovp) result += "VBUS OVP; ";      // Input bus overvoltage
  if (fault0.ibat_regulation) result += "IBAT REG; ";      // Battery regulation
  
  // Decode fault1 register (corrected to match BQ25798 datasheet)
  if (fault1.thermal_shutdown) result += "TSHUT; ";         // Thermal shutdown
  if (fault1.otg_uvp) result += "OTG UVP; ";       // OTG undervoltage
  if (fault1.otg_ovp) result += "OTG OVP; ";       // OTG overvoltage
  if (fault1.vsys_ovp) result += "VSYS OVP; ";      // System overvoltage
  if (fault1.vsys_short) result += "VSYS SHORT; ";    // System short
  
  if (result.endsWith("; ")) {
    result = result.substring(0, result.length() - 2);
  }
  
  return result;
}

void ModbeeMpptAPI::getAllChargerStatus(uint8_t* status0, uint8_t* status1, uint8_t* status2, 
                                     uint8_t* status3, uint8_t* status4) {
  if (status0) *status0 = _mppt._bq25798.getChargerStatus0();
  if (status1) *status1 = _mppt._bq25798.getChargerStatus1();
  if (status2) *status2 = _mppt._bq25798.getChargerStatus2();
  if (status3) *status3 = _mppt._bq25798.getChargerStatus3();
  if (status4) *status4 = _mppt._bq25798.getChargerStatus4();
}

bool ModbeeMpptAPI::isInInputPowerManagement() {
  modbee_status0_t status0 = getStatus0();
  return status0.iindpm_active || status0.vindpm_active;
}

bool ModbeeMpptAPI::isPowerGood() {
  modbee_status0_t status0 = getStatus0();
  return status0.power_good;
}

bool ModbeeMpptAPI::isVbusPresent() {
  modbee_status0_t status0 = getStatus0();
  return status0.vbus_present;
}

bool ModbeeMpptAPI::isBatteryPresent() {
  modbee_status2_t status2 = getStatus2();
  return status2.battery_present;
}

bool ModbeeMpptAPI::isInThermalRegulation() {
  modbee_status2_t status2 = getStatus2();
  return status2.thermal_regulation;
}

bool ModbeeMpptAPI::isICOComplete() {
  modbee_status2_t status2 = getStatus2();
  return status2.ico_status == MODBEE_ICO_MAX_CURRENT;
}

bool ModbeeMpptAPI::isADCComplete() {
  modbee_status3_t status3 = getStatus3();
  return status3.adc_conversion_done;
}

void ModbeeMpptAPI::getAllFaultStatus(uint8_t* fault0, uint8_t* fault1) {
  if (fault0) *fault0 = _mppt._bq25798.getFaultStatus0();
  if (fault1) *fault1 = _mppt._bq25798.getFaultStatus1();
}

bool ModbeeMpptAPI::hasInputOvervoltageFault() {
  modbee_fault0_t fault0 = getFault0();
  return fault0.vac1_ovp || fault0.vac2_ovp || fault0.vbus_ovp;
}

bool ModbeeMpptAPI::hasOvercurrentFault() {
  modbee_fault0_t fault0 = getFault0();
  return fault0.converter_ocp || fault0.ibat_ocp || fault0.ibus_ocp;
}

bool ModbeeMpptAPI::hasThermalFault() {
  modbee_fault1_t fault1 = getFault1();
  modbee_status2_t status2 = getStatus2();
  return fault1.thermal_shutdown || status2.thermal_regulation;
}

// ========================================================================
// PRIVATE HELPER FUNCTIONS
// ========================================================================

float ModbeeMpptAPI::clampValue(float value, float min_val, float max_val) {
  if (value < min_val) return min_val;
  if (value > max_val) return max_val;
  return value;
}

float ModbeeMpptAPI::calculateBatterySOC(float voltage) {
  if (voltage < _battery_min_voltage) return 0.0f;
  if (voltage > _battery_max_voltage) return 100.0f;
  
  float range = _battery_max_voltage - _battery_min_voltage;
  float position = voltage - _battery_min_voltage;
  
  return (position / range) * 100.0f;
}

bool ModbeeMpptAPI::setChargeEnable(bool enable) {
  return _mppt._bq25798.setChargeEnable(enable);
}

bool ModbeeMpptAPI::getChargeEnable() {
  return _mppt._bq25798.getChargeEnable();
}

float ModbeeMpptAPI::getChargeVoltage() {
  return _mppt._bq25798.getChargeLimitV();
}

float ModbeeMpptAPI::getChargeCurrent() {
  return _mppt._bq25798.getChargeLimitA();
}

float ModbeeMpptAPI::getInputCurrentLimit() {
  return _mppt._bq25798.getInputLimitA();
}

bool ModbeeMpptAPI::setHIZMode(bool enable) {
  return _mppt._bq25798.setHIZMode(enable);
}

bool ModbeeMpptAPI::getHIZMode() {
  return _mppt._bq25798.getHIZMode();
}

bool ModbeeMpptAPI::setBackupMode(bool enable) {
  return _mppt._bq25798.setBackupModeEnable(enable);
}

bool ModbeeMpptAPI::getBackupMode() {
  return _mppt._bq25798.getBackupModeEnable();
}

bool ModbeeMpptAPI::setMPPTVOCPercent(modbee_voc_percent_t percent) {
  return _mppt._bq25798.setVINDPM_VOCpercent((bq25798_voc_pct_t)percent);
}

modbee_voc_percent_t ModbeeMpptAPI::getMPPTVOCPercent() {
  return (modbee_voc_percent_t)_mppt._bq25798.getVINDPM_VOCpercent();
}

bool ModbeeMpptAPI::setMPPTVOCDelay(modbee_voc_delay_t delay) {
  return _mppt._bq25798.setVOCdelay((bq25798_voc_dly_t)delay);
}

bool ModbeeMpptAPI::setMPPTVOCRate(modbee_voc_rate_t rate) {
  return _mppt._bq25798.setVOCrate((bq25798_voc_rate_t)rate);
}

// ========================================================================
// UTILITY FUNCTIONS
// ========================================================================

float ModbeeMpptAPI::vocPercentToFloat(modbee_voc_percent_t voc_enum) {
  switch (voc_enum) {
    case MODBEE_VOC_PERCENT_56_25: return 56.25f;
    case MODBEE_VOC_PERCENT_62_5:  return 62.5f;
    case MODBEE_VOC_PERCENT_68_75: return 68.75f;
    case MODBEE_VOC_PERCENT_75:    return 75.0f;
    case MODBEE_VOC_PERCENT_81_25: return 81.25f;
    case MODBEE_VOC_PERCENT_87_5:  return 87.5f;
    case MODBEE_VOC_PERCENT_93_75: return 93.75f;
    case MODBEE_VOC_PERCENT_100:   return 100.0f;
    default: return 87.5f; // Default value
  }
}

modbee_voc_percent_t ModbeeMpptAPI::floatToVocPercent(float percentage) {
  if (percentage <= 59.375f) return MODBEE_VOC_PERCENT_56_25;
  else if (percentage <= 65.625f) return MODBEE_VOC_PERCENT_62_5;
  else if (percentage <= 71.875f) return MODBEE_VOC_PERCENT_68_75;
  else if (percentage <= 78.125f) return MODBEE_VOC_PERCENT_75;
  else if (percentage <= 84.375f) return MODBEE_VOC_PERCENT_81_25;
  else if (percentage <= 90.625f) return MODBEE_VOC_PERCENT_87_5;
  else if (percentage <= 96.875f) return MODBEE_VOC_PERCENT_93_75;
  else return MODBEE_VOC_PERCENT_100;
}

// ========================================================================
// STRUCTURED STATUS ACCESS FUNCTIONS (Datasheet-accurate structs)
// ========================================================================

modbee_complete_status_t ModbeeMpptAPI::getCompleteStatus() {
  modbee_complete_status_t status;
  status.status0 = getStatus0();
  status.status1 = getStatus1();
  status.status2 = getStatus2();
  status.status3 = getStatus3();
  status.status4 = getStatus4();
  status.fault0 = getFault0();
  status.fault1 = getFault1();
  return status;
}

modbee_status0_t ModbeeMpptAPI::getStatus0() {
  uint8_t reg = _mppt._bq25798.getChargerStatus0();
  
  modbee_status0_t status = {};
  status.vbus_present = (reg & 0x01) != 0;       // Bit 0
  status.ac1_present = (reg & 0x02) != 0;        // Bit 1
  status.ac2_present = (reg & 0x04) != 0;        // Bit 2
  status.power_good = (reg & 0x08) != 0;         // Bit 3
  // Bit 4: Reserved
  status.watchdog_expired = (reg & 0x20) != 0;   // Bit 5
  status.vindpm_active = (reg & 0x40) != 0;      // Bit 6
  status.iindpm_active = (reg & 0x80) != 0;      // Bit 7
  
  return status;
}

modbee_status1_t ModbeeMpptAPI::getStatus1() {
  uint8_t reg = _mppt._bq25798.getChargerStatus1();
  
  modbee_status1_t status = {};
  status.bc12_done = (reg & 0x01) != 0;                           // Bit 0
  status.vbus_status = (modbee_vbus_status_t)((reg >> 1) & 0x0F); // Bits 4:1
  status.charge_state = (modbee_charge_state_t)((reg >> 5) & 0x07); // Bits 7:5
  
  return status;
}

modbee_status2_t ModbeeMpptAPI::getStatus2() {
  uint8_t reg = _mppt._bq25798.getChargerStatus2();
  
  modbee_status2_t status = {};
  status.battery_present = (reg & 0x01) != 0;        // Bit 0
  status.dpdm_detection_ongoing = (reg & 0x02) != 0; // Bit 1
  status.thermal_regulation = (reg & 0x04) != 0;     // Bit 2
  // Bits 5:3: Reserved
  status.ico_status = (modbee_ico_status_t)((reg >> 6) & 0x03); // Bits 7:6
  
  return status;
}

modbee_status3_t ModbeeMpptAPI::getStatus3() {
  uint8_t reg = _mppt._bq25798.getChargerStatus3();
  
  modbee_status3_t status = {};
  // Bit 0: Reserved
  status.precharge_timer_expired = (reg & 0x02) != 0;  // Bit 1
  status.trickle_timer_expired = (reg & 0x04) != 0;    // Bit 2
  status.charge_timer_expired = (reg & 0x08) != 0;     // Bit 3
  status.vsys_regulation = (reg & 0x10) != 0;          // Bit 4
  status.adc_conversion_done = (reg & 0x20) != 0;      // Bit 5
  status.acrb1_active = (reg & 0x40) != 0;             // Bit 6
  status.acrb2_active = (reg & 0x80) != 0;             // Bit 7
  
  return status;
}

modbee_status4_t ModbeeMpptAPI::getStatus4() {
  uint8_t reg = _mppt._bq25798.getChargerStatus4();
  
  modbee_status4_t status = {};
  status.ts_hot = (reg & 0x01) != 0;           // Bit 0
  status.ts_warm = (reg & 0x02) != 0;          // Bit 1
  status.ts_cool = (reg & 0x04) != 0;          // Bit 2
  status.ts_cold = (reg & 0x08) != 0;          // Bit 3
  status.vbat_otg_low = (reg & 0x10) != 0;     // Bit 4
  // Bits 7:5: Reserved
  
  return status;
}

modbee_fault0_t ModbeeMpptAPI::getFault0() {
  uint8_t reg = _mppt._bq25798.getFaultStatus0();
  
  modbee_fault0_t fault = {};
  fault.vac1_ovp = (reg & 0x01) != 0;          // Bit 0
  fault.vac2_ovp = (reg & 0x02) != 0;          // Bit 1
  fault.converter_ocp = (reg & 0x04) != 0;     // Bit 2
  fault.ibat_ocp = (reg & 0x08) != 0;          // Bit 3
  fault.ibus_ocp = (reg & 0x10) != 0;          // Bit 4
  fault.vbat_ovp = (reg & 0x20) != 0;          // Bit 5 - VBAT_OVP (Battery overvoltage)
  fault.vbus_ovp = (reg & 0x40) != 0;          // Bit 6 - VBUS_OVP (Input overvoltage)
  fault.ibat_regulation = (reg & 0x80) != 0;   // Bit 7
  
  return fault;
}

modbee_fault1_t ModbeeMpptAPI::getFault1() {
  uint8_t reg = _mppt._bq25798.getFaultStatus1();
  
  modbee_fault1_t fault = {};
  // Bits 1:0: Reserved
  fault.thermal_shutdown = (reg & 0x04) != 0;  // Bit 2: TSHUT_STAT
  // Bit 3: Reserved
  fault.otg_uvp = (reg & 0x10) != 0;           // Bit 4: OTG_UVP_STAT
  fault.otg_ovp = (reg & 0x20) != 0;           // Bit 5: OTG_OVP_STAT
  fault.vsys_ovp = (reg & 0x40) != 0;          // Bit 6: VSYS_OVP_STAT
  fault.vsys_short = (reg & 0x80) != 0;        // Bit 7: VSYS_SHORT_STAT
  
  return fault;
}

float ModbeeMpptAPI::getRawTSPercent() {
  return _mppt._bq25798.getADCTS(); // Raw TS ADC reading as percentage
}
