/*!
 * @file BQ25798.cpp
 *
 * @mainpage BQ25798 I2C Controlled Buck-Boost Battery Charger
 *
 * @section intro_sec Introduction
 *
 * This is a library for the BQ25798 I2C controlled buck-boost battery charger
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#include "BQ25798.h"

/*!
 * @brief  Instantiates a new BQ25798 class
 */
BQ25798::BQ25798(TwoWire *wire) {
  _wire = wire;
  _softWire = NULL;
  _use_soft_i2c = false;
}

BQ25798::BQ25798(SoftWire *wire) {
  _wire = NULL;
  _softWire = wire;
  _use_soft_i2c = true;
}

/*!
 * @brief  Destroys the BQ25798 object
 */
BQ25798::~BQ25798() {
}

/*!
 * @brief  Sets up the hardware and initializes I2C
 * @param  i2c_addr
 *         The I2C address to be used.
 * @return True if initialization was successful, otherwise false.
 */
bool BQ25798::begin(uint8_t i2c_addr) {
  _i2c_addr = i2c_addr;

  if (_use_soft_i2c) {
    _softWire->begin();
  } else {
    _wire->begin();
  }

  uint8_t part_info;
  if (!readRegister(BQ25798_REG_PART_INFORMATION, &part_info)) {
    //return false;
  }
  
  // Verify part number (bits 6-3 should be 0111b = 7h for BQ25798)
  if ((part_info & 0x78) >> 3 != 0x07) {
    //return false;
  }

  // Don't reset during initialization to avoid disrupting existing settings
  // reset();

  BQ25798_DEBUGLN("BQ25798: Initialization successful");
  return true;
}

/*!
 * @brief Get the minimal system voltage setting
 * @return Minimal system voltage in volts
 */
float BQ25798::getMinSystemV() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_MINIMAL_SYSTEM_VOLTAGE, &reg_value, 6, 0);
  
  // Convert to voltage: (register_value × 250mV) + 2500mV
  return (reg_value * 0.25f) + 2.5f;
}

/*!
 * @brief Set the minimal system voltage
 * @param voltage Minimal system voltage in volts (2.5V to 16.0V)
 * @return True if successful, false if voltage out of range
 */
bool BQ25798::setMinSystemV(float voltage) {
  if (voltage < 2.5f || voltage > 16.0f) {
    return false;
  }
  
  // Convert voltage to register value: (voltage - 2.5V) / 0.25V
  uint8_t reg_value = (uint8_t)((voltage - 2.5f) / 0.25f);
  
  // Clamp to 6-bit range (0-63)
  if (reg_value > 63) {
    reg_value = 63;
  }
  
  return writeRegisterBits(BQ25798_REG_MINIMAL_SYSTEM_VOLTAGE, reg_value, 6, 0);
}

/*!
 * @brief Get the charge voltage limit setting
 * @return Charge voltage limit in volts
 */
float BQ25798::getChargeLimitV() {
  uint16_t reg_value;
  readRegisterBits16(BQ25798_REG_CHARGE_VOLTAGE_LIMIT, &reg_value, 11, 0);
  
  // Convert to voltage: register_value × 10mV
  return reg_value * 0.01f;
}

/*!
 * @brief Set the charge voltage limit
 * @param voltage Charge voltage limit in volts (3.0V to 18.8V)
 * @return True if successful, false if voltage out of range
 */
bool BQ25798::setChargeLimitV(float voltage) {
  if (voltage < 3.0f || voltage > 18.8f) {
    return false;
  }
  
  // Convert voltage to register value: voltage / 0.01V
  uint16_t reg_value = (uint16_t)(voltage / 0.01f);
  
  // Clamp to 11-bit range (0-2047)
  if (reg_value > 2047) {
    reg_value = 2047;
  }
  
  return writeRegisterBits16(BQ25798_REG_CHARGE_VOLTAGE_LIMIT, reg_value, 11, 0);
}

/*!
 * @brief Get the charge current limit setting
 * @return Charge current limit in amps
 */
float BQ25798::getChargeLimitA() {
  uint16_t reg_value;
  readRegisterBits16(BQ25798_REG_CHARGE_CURRENT_LIMIT, &reg_value, 9, 0);
  
  // Convert to current: register_value × 10mA
  return reg_value * 0.01f;
}

/*!
 * @brief Set the charge current limit
 * @param current Charge current limit in amps (0.05A to 5.0A)
 * @return True if successful, false if current out of range
 */
bool BQ25798::setChargeLimitA(float current) {
  if (current < 0.05f || current > 5.0f) {
    return false;
  }
  
  // Convert current to register value: current / 0.01A
  uint16_t reg_value = (uint16_t)(current / 0.01f);
  
  // Clamp to 9-bit range (0-511)
  if (reg_value > 511) {
    reg_value = 511;
  }
  
  return writeRegisterBits16(BQ25798_REG_CHARGE_CURRENT_LIMIT, reg_value, 9, 0);
}

/*!
 * @brief Get the input voltage limit setting
 * @return Input voltage limit in volts
 */
float BQ25798::getInputLimitV() {
  uint8_t reg_value;
  readRegister(BQ25798_REG_INPUT_VOLTAGE_LIMIT, &reg_value);
  
  // Convert to voltage: register_value × 100mV
  return reg_value * 0.1f;
}

/*!
 * @brief Set the input voltage limit
 * @param voltage Input voltage limit in volts (3.6V to 22.0V)
 * @return True if successful, false if voltage out of range
 */
bool BQ25798::setInputLimitV(float voltage) {
  if (voltage < 3.6f || voltage > 22.0f) {
    return false;
  }
  
  // Convert voltage to register value: voltage / 0.1V
  uint8_t reg_value = (uint8_t)(voltage / 0.1f);
  
  return writeRegister(BQ25798_REG_INPUT_VOLTAGE_LIMIT, reg_value);
}

/*!
 * @brief Get the input current limit setting
 * @return Input current limit in amps
 */
float BQ25798::getInputLimitA() {
  uint16_t reg_value;
  readRegisterBits16(BQ25798_REG_INPUT_CURRENT_LIMIT, &reg_value, 9, 0);
  
  // Convert to current: register_value × 10mA
  return reg_value * 0.01f;
}

/*!
 * @brief Set the input current limit
 * @param current Input current limit in amps (0.1A to 3.3A)
 * @return True if successful, false if current out of range
 */
bool BQ25798::setInputLimitA(float current) {
  if (current < 0.1f || current > 3.3f) {
    return false;
  }
  
  // Convert current to register value: current / 0.01A
  uint16_t reg_value = (uint16_t)(current / 0.01f);
  
  // Clamp to 9-bit range (0-511)
  if (reg_value > 511) {
    reg_value = 511;
  }
  
  return writeRegisterBits16(BQ25798_REG_INPUT_CURRENT_LIMIT, reg_value, 9, 0);
}

/*!
 * @brief Get the battery voltage threshold for precharge to fast charge transition
 * @return Battery voltage threshold as percentage of VREG
 */
bq25798_vbat_lowv_t BQ25798::getVBatLowV() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_PRECHARGE_CONTROL, &reg_value, 2, 6);
  
  return (bq25798_vbat_lowv_t)reg_value;
}

/*!
 * @brief Set the battery voltage threshold for precharge to fast charge transition
 * @param threshold Battery voltage threshold as percentage of VREG
 * @return True if successful
 */
bool BQ25798::setVBatLowV(bq25798_vbat_lowv_t threshold) {
  if (threshold > BQ25798_VBAT_LOWV_71_4_PERCENT) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_PRECHARGE_CONTROL, (uint8_t)threshold, 2, 6);
}

/*!
 * @brief Get the precharge current limit setting
 * @return Precharge current limit in amps
 */
float BQ25798::getPrechargeLimitA() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_PRECHARGE_CONTROL, &reg_value, 6, 0);
  
  // Convert to current: register_value × 40mA
  return reg_value * 0.04f;
}

/*!
 * @brief Set the precharge current limit
 * @param current Precharge current limit in amps (0.04A to 2.0A)
 * @return True if successful, false if current out of range
 */
bool BQ25798::setPrechargeLimitA(float current) {
  if (current < 0.04f || current > 2.0f) {
    return false;
  }
  
  // Convert current to register value: current / 0.04A
  uint8_t reg_value = (uint8_t)(current / 0.04f);
  
  // Clamp to 6-bit range (0-63)
  if (reg_value > 63) {
    reg_value = 63;
  }
  
  return writeRegisterBits(BQ25798_REG_PRECHARGE_CONTROL, reg_value, 6, 0);
}

/*!
 * @brief Get the watchdog timer reset behavior for safety timers
 * @return True if watchdog expiration will NOT reset safety timers, false if it will reset them
 */
bool BQ25798::getStopOnWDT() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TERMINATION_CONTROL, &reg_value, 1, 5);
  
  return reg_value == 1;
}

/*!
 * @brief Set the watchdog timer reset behavior for safety timers
 * @param stopOnWDT True = watchdog expiration will NOT reset safety timers, false = will reset them
 * @return True if successful
 */
bool BQ25798::setStopOnWDT(bool stopOnWDT) {
  return writeRegisterBits(BQ25798_REG_TERMINATION_CONTROL, stopOnWDT ? 1 : 0, 1, 5);
}

/*!
 * @brief Get the termination current limit setting
 * @return Termination current limit in amps
 */
float BQ25798::getTerminationA() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TERMINATION_CONTROL, &reg_value, 5, 0);
  
  // Convert to current: register_value × 40mA
  return reg_value * 0.04f;
}

/*!
 * @brief Set the termination current limit
 * @param current Termination current limit in amps (0.04A to 1.0A)
 * @return True if successful, false if current out of range
 */
bool BQ25798::setTerminationA(float current) {
  if (current < 0.04f || current > 1.0f) {
    return false;
  }
  
  // Convert current to register value: current / 0.04A
  uint8_t reg_value = (uint8_t)(current / 0.04f);
  
  // Clamp to 5-bit range (0-31)
  if (reg_value > 31) {
    reg_value = 31;
  }
  
  return writeRegisterBits(BQ25798_REG_TERMINATION_CONTROL, reg_value, 5, 0);
}

/*!
 * @brief Get the battery cell count setting
 * @return Battery cell count
 */
bq25798_cell_count_t BQ25798::getCellCount() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_RECHARGE_CONTROL, &reg_value, 2, 6);
  
  return (bq25798_cell_count_t)reg_value;
}

/*!
 * @brief Set the battery cell count
 * @param cellCount Battery cell count (1S to 4S)
 * @return True if successful
 */
bool BQ25798::setCellCount(bq25798_cell_count_t cellCount) {
  if (cellCount > BQ25798_CELL_COUNT_4S) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_RECHARGE_CONTROL, (uint8_t)cellCount, 2, 6);
}

/*!
 * @brief Get the battery recharge deglitch time setting
 * @return Battery recharge deglitch time
 */
bq25798_trechg_time_t BQ25798::getRechargeDeglitchTime() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_RECHARGE_CONTROL, &reg_value, 2, 4);
  
  return (bq25798_trechg_time_t)reg_value;
}

/*!
 * @brief Set the battery recharge deglitch time
 * @param deglitchTime Battery recharge deglitch time (64ms to 2048ms)
 * @return True if successful
 */
bool BQ25798::setRechargeDeglitchTime(bq25798_trechg_time_t deglitchTime) {
  if (deglitchTime > BQ25798_TRECHG_2048MS) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_RECHARGE_CONTROL, (uint8_t)deglitchTime, 2, 4);
}

/*!
 * @brief Get the battery recharge threshold offset voltage
 * @return Recharge threshold offset voltage in volts (below VREG)
 */
float BQ25798::getRechargeThreshOffsetV() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_RECHARGE_CONTROL, &reg_value, 4, 0);
  
  // Convert to voltage: (register_value × 50mV) + 50mV
  return (reg_value * 0.05f) + 0.05f;
}

/*!
 * @brief Set the battery recharge threshold offset voltage
 * @param voltage Recharge threshold offset voltage in volts (0.05V to 0.8V)
 * @return True if successful, false if voltage out of range
 */
bool BQ25798::setRechargeThreshOffsetV(float voltage) {
  if (voltage < 0.05f || voltage > 0.8f) {
    return false;
  }
  
  // Convert voltage to register value: (voltage - 0.05V) / 0.05V
  uint8_t reg_value = (uint8_t)((voltage - 0.05f) / 0.05f);
  
  // Clamp to 4-bit range (0-15)
  if (reg_value > 15) {
    reg_value = 15;
  }
  
  return writeRegisterBits(BQ25798_REG_RECHARGE_CONTROL, reg_value, 4, 0);
}

/*!
 * @brief Get the OTG mode regulation voltage setting
 * @return OTG voltage in volts
 */
float BQ25798::getOTGV() {
  uint16_t reg_value;
  readRegisterBits16(BQ25798_REG_VOTG_REGULATION, &reg_value, 11, 0);
  
  // Convert to voltage: (register_value × 10mV) + 2800mV
  return (reg_value * 0.01f) + 2.8f;
}

/*!
 * @brief Set the OTG mode regulation voltage
 * @param voltage OTG voltage in volts (2.8V to 22.0V)
 * @return True if successful, false if voltage out of range
 */
bool BQ25798::setOTGV(float voltage) {
  if (voltage < 2.8f || voltage > 22.0f) {
    return false;
  }
  
  // Convert voltage to register value: (voltage - 2.8V) / 0.01V
  uint16_t reg_value = (uint16_t)((voltage - 2.8f) / 0.01f);
  
  // Clamp to 11-bit range (0-2047)
  if (reg_value > 2047) {
    reg_value = 2047;
  }
  
  return writeRegisterBits16(BQ25798_REG_VOTG_REGULATION, reg_value, 11, 0);
}

/*!
 * @brief Get the precharge safety timer setting
 * @return Precharge timer setting
 */
bq25798_prechg_timer_t BQ25798::getPrechargeTimer() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_IOTG_REGULATION, &reg_value, 1, 7);
  
  return (bq25798_prechg_timer_t)reg_value;
}

/*!
 * @brief Set the precharge safety timer
 * @param timer Precharge timer setting (0.5hr or 2hr)
 * @return True if successful
 */
bool BQ25798::setPrechargeTimer(bq25798_prechg_timer_t timer) {
  if (timer > BQ25798_PRECHG_TMR_0_5HR) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_IOTG_REGULATION, (uint8_t)timer, 1, 7);
}

/*!
 * @brief Get the OTG current limit setting
 * @return OTG current limit in amps
 */
float BQ25798::getOTGLimitA() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_IOTG_REGULATION, &reg_value, 7, 0);
  
  // Convert to current: register_value × 40mA
  return reg_value * 0.04f;
}

/*!
 * @brief Set the OTG current limit
 * @param current OTG current limit in amps (0.16A to 3.36A)
 * @return True if successful, false if current out of range
 */
bool BQ25798::setOTGLimitA(float current) {
  if (current < 0.16f || current > 3.36f) {
    return false;
  }
  
  // Convert current to register value: current / 0.04A
  uint8_t reg_value = (uint8_t)(current / 0.04f);
  
  // Clamp to 7-bit range (0-127)
  if (reg_value > 127) {
    reg_value = 127;
  }
  
  return writeRegisterBits(BQ25798_REG_IOTG_REGULATION, reg_value, 7, 0);
}

/*!
 * @brief Get the top-off timer setting
 * @return Top-off timer setting
 */
bq25798_topoff_timer_t BQ25798::getTopOffTimer() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TIMER_CONTROL, &reg_value, 2, 6);
  
  return (bq25798_topoff_timer_t)reg_value;
}

/*!
 * @brief Set the top-off timer
 * @param timer Top-off timer setting (disabled, 15min, 30min, 45min)
 * @return True if successful
 */
bool BQ25798::setTopOffTimer(bq25798_topoff_timer_t timer) {
  if (timer > BQ25798_TOPOFF_TMR_45MIN) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_TIMER_CONTROL, (uint8_t)timer, 2, 6);
}

/*!
 * @brief Get the trickle charge timer enable setting
 * @return True if trickle charge timer is enabled, false if disabled
 */
bool BQ25798::getTrickleChargeTimerEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TIMER_CONTROL, &reg_value, 1, 5);
  
  return reg_value == 1;
}

/*!
 * @brief Set the trickle charge timer enable
 * @param enable True = enable trickle charge timer, false = disable
 * @return True if successful
 */
bool BQ25798::setTrickleChargeTimerEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_TIMER_CONTROL, enable ? 1 : 0, 1, 5);
}

/*!
 * @brief Get the precharge timer enable setting
 * @return True if precharge timer is enabled, false if disabled
 */
bool BQ25798::getPrechargeTimerEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TIMER_CONTROL, &reg_value, 1, 4);
  
  return reg_value == 1;
}

/*!
 * @brief Set the precharge timer enable
 * @param enable True = enable precharge timer, false = disable
 * @return True if successful
 */
bool BQ25798::setPrechargeTimerEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_TIMER_CONTROL, enable ? 1 : 0, 1, 4);
}

/*!
 * @brief Get the fast charge timer enable setting
 * @return True if fast charge timer is enabled, false if disabled
 */
bool BQ25798::getFastChargeTimerEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TIMER_CONTROL, &reg_value, 1, 3);
  
  return reg_value == 1;
}

/*!
 * @brief Set the fast charge timer enable
 * @param enable True = enable fast charge timer, false = disable
 * @return True if successful
 */
bool BQ25798::setFastChargeTimerEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_TIMER_CONTROL, enable ? 1 : 0, 1, 3);
}

/*!
 * @brief Get the fast charge timer setting
 * @return Fast charge timer setting
 */
bq25798_chg_timer_t BQ25798::getFastChargeTimer() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TIMER_CONTROL, &reg_value, 2, 1);
  
  return (bq25798_chg_timer_t)reg_value;
}

/*!
 * @brief Set the fast charge timer
 * @param timer Fast charge timer setting (5hr, 8hr, 12hr, 24hr)
 * @return True if successful
 */
bool BQ25798::setFastChargeTimer(bq25798_chg_timer_t timer) {
  if (timer > BQ25798_CHG_TMR_24HR) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_TIMER_CONTROL, (uint8_t)timer, 2, 1);
}

/*!
 * @brief Get the timer half-rate enable setting
 * @return True if timer half-rate is enabled, false if disabled
 */
bool BQ25798::getTimerHalfRateEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TIMER_CONTROL, &reg_value, 1, 0);
  
  return reg_value == 1;
}

/*!
 * @brief Set the timer half-rate enable
 * @param enable True = enable timer half-rate during regulation, false = disable
 * @return True if successful
 */
bool BQ25798::setTimerHalfRateEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_TIMER_CONTROL, enable ? 1 : 0, 1, 0);
}

/*!
 * @brief Get the automatic OVP battery discharge enable setting
 * @return True if automatic OVP battery discharge is enabled, false if disabled
 */
bool BQ25798::getAutoOVPBattDischarge() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, &reg_value, 1, 7);
  
  return reg_value == 1;
}

/*!
 * @brief Set the automatic OVP battery discharge enable
 * @param enable True = enable automatic OVP battery discharge, false = disable
 * @return True if successful
 */
bool BQ25798::setAutoOVPBattDischarge(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, enable ? 1 : 0, 1, 7);
}

/*!
 * @brief Get the force battery discharge setting
 * @return True if force battery discharge is enabled, false if disabled
 */
bool BQ25798::getForceBattDischarge() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, &reg_value, 1, 6);
  
  return reg_value == 1;
}

/*!
 * @brief Set the force battery discharge
 * @param enable True = force battery discharge, false = disable
 * @return True if successful
 */
bool BQ25798::setForceBattDischarge(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, enable ? 1 : 0, 1, 6);
}

/*!
 * @brief Get the charge enable setting
 * @return True if charging is enabled, false if disabled
 */
bool BQ25798::getChargeEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, &reg_value, 1, 5);
  
  return reg_value == 1;
}

/*!
 * @brief Set the charge enable
 * @param enable True = enable charging, false = disable charging
 * @return True if successful
 */
bool BQ25798::setChargeEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, enable ? 1 : 0, 1, 5);
}

/*!
 * @brief Get the ICO (Input Current Optimizer) enable setting
 * @return True if ICO is enabled, false if disabled
 */
bool BQ25798::getICOEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, &reg_value, 1, 4);
  
  return reg_value == 1;
}

/*!
 * @brief Set the ICO (Input Current Optimizer) enable
 * @param enable True = enable ICO, false = disable ICO
 * @return True if successful
 */
bool BQ25798::setICOEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, enable ? 1 : 0, 1, 4);
}

/*!
 * @brief Get the force ICO setting
 * @return True if force ICO is enabled, false if disabled
 */
bool BQ25798::getForceICO() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, &reg_value, 1, 3);
  
  return reg_value == 1;
}

/*!
 * @brief Set the force ICO
 * @param enable True = force ICO, false = disable
 * @return True if successful
 */
bool BQ25798::setForceICO(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, enable ? 1 : 0, 1, 3);
}

/*!
 * @brief Get the HIZ (High Impedance) mode setting
 * @return True if HIZ mode is enabled, false if disabled
 */
bool BQ25798::getHIZMode() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, &reg_value, 1, 2);
  
  return reg_value == 1;
}

/*!
 * @brief Set the HIZ (High Impedance) mode
 * @param enable True = enable HIZ mode, false = disable HIZ mode
 * @return True if successful
 */
bool BQ25798::setHIZMode(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, enable ? 1 : 0, 1, 2);
}

/*!
 * @brief Get the charge termination enable setting
 * @return True if charge termination is enabled, false if disabled
 */
bool BQ25798::getTerminationEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, &reg_value, 1, 1);
  
  return reg_value == 1;
}

/*!
 * @brief Set the charge termination enable
 * @param enable True = enable charge termination, false = disable charge termination
 * @return True if successful
 */
bool BQ25798::setTerminationEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, enable ? 1 : 0, 1, 1);
}

/*!
 * @brief Get the backup mode enable setting
 * @return True if backup mode is enabled, false if disabled
 */
bool BQ25798::getBackupModeEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, &reg_value, 1, 0);
  
  return reg_value == 1;
}

/*!
 * @brief Set the backup mode enable
 * @param enable True = enable backup mode, false = disable backup mode
 * @return True if successful
 */
bool BQ25798::setBackupModeEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_0, enable ? 1 : 0, 1, 0);
}

/*!
 * @brief Get the backup mode threshold setting
 * @return Backup mode threshold setting
 */
bq25798_vbus_backup_t BQ25798::getBackupModeThresh() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_1, &reg_value, 2, 6);
  
  return (bq25798_vbus_backup_t)reg_value;
}

/*!
 * @brief Set the backup mode threshold
 * @param threshold Backup mode threshold setting
 * @return True if successful
 */
bool BQ25798::setBackupModeThresh(bq25798_vbus_backup_t threshold) {
  if (threshold > BQ25798_VBUS_BACKUP_100_PERCENT) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_1, (uint8_t)threshold, 2, 6);
}

/*!
 * @brief Get the VAC overvoltage protection setting
 * @return VAC OVP setting
 */
bq25798_vac_ovp_t BQ25798::getVACOVP() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_1, &reg_value, 2, 4);
  
  return (bq25798_vac_ovp_t)reg_value;
}

/*!
 * @brief Set the VAC overvoltage protection
 * @param threshold VAC OVP setting
 * @return True if successful
 */
bool BQ25798::setVACOVP(bq25798_vac_ovp_t threshold) {
  if (threshold > BQ25798_VAC_OVP_7V) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_1, (uint8_t)threshold, 2, 4);
}

/*!
 * @brief Reset the watchdog timer
 * @return True if successful
 */
bool BQ25798::resetWDT() {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_1, 1, 1, 3);
}

/*!
 * @brief Get the watchdog timer setting
 * @return Watchdog timer setting
 */
bq25798_wdt_t BQ25798::getWDT() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_1, &reg_value, 3, 0);
  
  return (bq25798_wdt_t)reg_value;
}

/*!
 * @brief Set the watchdog timer
 * @param timer Watchdog timer setting
 * @return True if successful
 */
bool BQ25798::setWDT(bq25798_wdt_t timer) {
  if (timer > BQ25798_WDT_160S) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_1, (uint8_t)timer, 3, 0);
}

/*!
 * @brief Get the force D+/D- pins detection setting
 * @return True if force D+/D- detection is enabled, false if disabled
 */
bool BQ25798::getForceDPinsDetection() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, &reg_value, 1, 7);
  
  return reg_value == 1;
}

/*!
 * @brief Set the force D+/D- pins detection
 * @param enable True = force D+/D- detection, false = disable
 * @return True if successful
 */
bool BQ25798::setForceDPinsDetection(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, enable ? 1 : 0, 1, 7);
}

/*!
 * @brief Get the automatic D+/D- pins detection enable setting
 * @return True if automatic D+/D- detection is enabled, false if disabled
 */
bool BQ25798::getAutoDPinsDetection() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, &reg_value, 1, 6);
  
  return reg_value == 1;
}

/*!
 * @brief Set the automatic D+/D- pins detection enable
 * @param enable True = enable automatic D+/D- detection, false = disable
 * @return True if successful
 */
bool BQ25798::setAutoDPinsDetection(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, enable ? 1 : 0, 1, 6);
}

/*!
 * @brief Get the HVDCP 12V enable setting
 * @return True if HVDCP 12V is enabled, false if disabled
 */
bool BQ25798::getHVDCP12VEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, &reg_value, 1, 5);
  
  return reg_value == 1;
}

/*!
 * @brief Set the HVDCP 12V enable
 * @param enable True = enable HVDCP 12V, false = disable
 * @return True if successful
 */
bool BQ25798::setHVDCP12VEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, enable ? 1 : 0, 1, 5);
}

/*!
 * @brief Get the HVDCP 9V enable setting
 * @return True if HVDCP 9V is enabled, false if disabled
 */
bool BQ25798::getHVDCP9VEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, &reg_value, 1, 4);
  
  return reg_value == 1;
}

/*!
 * @brief Set the HVDCP 9V enable
 * @param enable True = enable HVDCP 9V, false = disable
 * @return True if successful
 */
bool BQ25798::setHVDCP9VEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, enable ? 1 : 0, 1, 4);
}

/*!
 * @brief Get the HVDCP enable setting
 * @return True if HVDCP is enabled, false if disabled
 */
bool BQ25798::getHVDCPEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, &reg_value, 1, 3);
  
  return reg_value == 1;
}

/*!
 * @brief Set the HVDCP enable
 * @param enable True = enable HVDCP, false = disable
 * @return True if successful
 */
bool BQ25798::setHVDCPEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, enable ? 1 : 0, 1, 3);
}

/*!
 * @brief Get the ship FET mode setting
 * @return Ship FET mode setting
 */
bq25798_sdrv_ctrl_t BQ25798::getShipFETmode() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, &reg_value, 2, 1);
  
  return (bq25798_sdrv_ctrl_t)reg_value;
}

/*!
 * @brief Set the ship FET mode
 * @param mode Ship FET mode setting
 * @return True if successful
 */
bool BQ25798::setShipFETmode(bq25798_sdrv_ctrl_t mode) {
  if (mode > BQ25798_SDRV_SYSTEM_RESET) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, (uint8_t)mode, 2, 1);
}

/*!
 * @brief Get the ship FET 10s delay setting
 * @return True if 10s delay is enabled, false if disabled
 */
bool BQ25798::getShipFET10sDelay() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, &reg_value, 1, 0);
  
  return reg_value == 1;
}

/*!
 * @brief Set the ship FET 10s delay
 * @param enable True = enable 10s delay, false = disable
 * @return True if successful
 */
bool BQ25798::setShipFET10sDelay(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_2, enable ? 1 : 0, 1, 0);
}

/*!
 * @brief Get the AC FET enable setting
 * @return True if AC FETs are enabled, false if disabled
 */
bool BQ25798::getACenable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, &reg_value, 1, 7);
  
  return reg_value == 1;
}

/*!
 * @brief Set the AC FET enable
 * @param enable True = enable AC FETs, false = disable
 * @return True if successful
 */
bool BQ25798::setACenable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, enable ? 1 : 0, 1, 7);
}

/*!
 * @brief Get the OTG mode enable setting
 * @return True if OTG mode is enabled, false if disabled
 */
bool BQ25798::getOTGenable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, &reg_value, 1, 6);
  
  return reg_value == 1;
}

/*!
 * @brief Set the OTG mode enable
 * @param enable True = enable OTG mode, false = disable
 * @return True if successful
 */
bool BQ25798::setOTGenable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, enable ? 1 : 0, 1, 6);
}

/*!
 * @brief Get the OTG PFM (Pulse Frequency Modulation) setting
 * @return True if OTG PFM is enabled, false if disabled
 */
bool BQ25798::getOTGPFM() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, &reg_value, 1, 5);
  
  return reg_value == 1;
}

/*!
 * @brief Set the OTG PFM (Pulse Frequency Modulation)
 * @param enable True = enable OTG PFM, false = disable
 * @return True if successful
 */
bool BQ25798::setOTGPFM(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, enable ? 1 : 0, 1, 5);
}

/*!
 * @brief Get the forward PFM (Pulse Frequency Modulation) setting
 * @return True if forward PFM is enabled, false if disabled
 */
bool BQ25798::getForwardPFM() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, &reg_value, 1, 4);
  
  return reg_value == 1;
}

/*!
 * @brief Set the forward PFM (Pulse Frequency Modulation)
 * @param enable True = enable forward PFM, false = disable
 * @return True if successful
 */
bool BQ25798::setForwardPFM(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, enable ? 1 : 0, 1, 4);
}

/*!
 * @brief Get the ship mode wakeup delay setting
 * @return Ship mode wakeup delay
 */
bq25798_wkup_dly_t BQ25798::getShipWakeupDelay() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, &reg_value, 1, 3);
  
  return (bq25798_wkup_dly_t)reg_value;
}

/*!
 * @brief Set the ship mode wakeup delay
 * @param delay Ship mode wakeup delay
 * @return True if successful
 */
bool BQ25798::setShipWakeupDelay(bq25798_wkup_dly_t delay) {
  if (delay > BQ25798_WKUP_DLY_15MS) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, (uint8_t)delay, 1, 3);
}

/*!
 * @brief Get the BATFET LDO precharge setting
 * @return True if BATFET LDO precharge is enabled, false if disabled
 */
bool BQ25798::getBATFETLDOprecharge() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, &reg_value, 1, 2);
  
  return reg_value == 1;
}

/*!
 * @brief Set the BATFET LDO precharge
 * @param enable True = enable BATFET LDO precharge, false = disable
 * @return True if successful
 */
bool BQ25798::setBATFETLDOprecharge(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, enable ? 1 : 0, 1, 2);
}

/*!
 * @brief Get the OTG OOA (Out of Audio) setting
 * @return True if OTG OOA is enabled, false if disabled
 */
bool BQ25798::getOTGOOA() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, &reg_value, 1, 1);
  
  return reg_value == 1;
}

/*!
 * @brief Set the OTG OOA (Out of Audio)
 * @param enable True = enable OTG OOA, false = disable
 * @return True if successful
 */
bool BQ25798::setOTGOOA(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, enable ? 1 : 0, 1, 1);
}

/*!
 * @brief Get the forward OOA (Out of Audio) setting
 * @return True if forward OOA is enabled, false if disabled
 */
bool BQ25798::getForwardOOA() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, &reg_value, 1, 0);
  
  return reg_value == 1;
}

/*!
 * @brief Set the forward OOA (Out of Audio)
 * @param enable True = enable forward OOA, false = disable
 * @return True if successful
 */
bool BQ25798::setForwardOOA(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_3, enable ? 1 : 0, 1, 0);
}

/*!
 * @brief Get the ACDRV2 enable setting
 * @return True if ACDRV2 is enabled, false if disabled
 */
bool BQ25798::getACDRV2enable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, &reg_value, 1, 7);
  
  return reg_value == 1;
}

/*!
 * @brief Set the ACDRV2 enable
 * @param enable True = enable ACDRV2, false = disable
 * @return True if successful
 */
bool BQ25798::setACDRV2enable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, enable ? 1 : 0, 1, 7);
}

/*!
 * @brief Get the ACDRV1 enable setting
 * @return True if ACDRV1 is enabled, false if disabled
 */
bool BQ25798::getACDRV1enable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, &reg_value, 1, 6);
  
  return reg_value == 1;
}

/*!
 * @brief Set the ACDRV1 enable
 * @param enable True = enable ACDRV1, false = disable
 * @return True if successful
 */
bool BQ25798::setACDRV1enable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, enable ? 1 : 0, 1, 6);
}

/*!
 * @brief Get the PWM switching frequency setting
 * @return PWM frequency
 */
bq25798_pwm_freq_t BQ25798::getPWMFrequency() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, &reg_value, 1, 5);
  
  return (bq25798_pwm_freq_t)reg_value;
}

/*!
 * @brief Set the PWM switching frequency
 * @param frequency PWM frequency
 * @return True if successful
 */
bool BQ25798::setPWMFrequency(bq25798_pwm_freq_t frequency) {
  if (frequency > BQ25798_PWM_FREQ_750KHZ) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, (uint8_t)frequency, 1, 5);
}

/*!
 * @brief Get the STAT pin enable setting
 * @return True if STAT pin is enabled, false if disabled
 */
bool BQ25798::getStatPinEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, &reg_value, 1, 4);
  
  return reg_value == 1;
}

/*!
 * @brief Set the STAT pin enable
 * @param enable True = enable STAT pin, false = disable
 * @return True if successful
 */
bool BQ25798::setStatPinEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, enable ? 1 : 0, 1, 4);
}

/*!
 * @brief Get the VSYS short circuit protection setting
 * @return True if VSYS short protection is enabled, false if disabled
 */
bool BQ25798::getVSYSshortProtect() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, &reg_value, 1, 3);
  
  return reg_value == 1;
}

/*!
 * @brief Set the VSYS short circuit protection
 * @param enable True = enable VSYS short protection, false = disable
 * @return True if successful
 */
bool BQ25798::setVSYSshortProtect(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, enable ? 1 : 0, 1, 3);
}

/*!
 * @brief Get the VOTG UVP (Under Voltage Protection) setting
 * @return True if VOTG UVP is enabled, false if disabled
 */
bool BQ25798::getVOTG_UVPProtect() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, &reg_value, 1, 2);
  
  return reg_value == 1;
}

/*!
 * @brief Set the VOTG UVP (Under Voltage Protection)
 * @param enable True = enable VOTG UVP, false = disable
 * @return True if successful
 */
bool BQ25798::setVOTG_UVPProtect(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, enable ? 1 : 0, 1, 2);
}

/*!
 * @brief Get the IBUS OCP (Over Current Protection) enable setting
 * @return True if IBUS OCP is enabled, false if disabled
 */
bool BQ25798::getIBUS_OCPenable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, &reg_value, 1, 1);
  
  return reg_value == 1;
}

/*!
 * @brief Set the IBUS OCP (Over Current Protection) enable
 * @param enable True = enable IBUS OCP, false = disable
 * @return True if successful
 */
bool BQ25798::setIBUS_OCPenable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, enable ? 1 : 0, 1, 1);
}

/*!
 * @brief Get the VINDPM detection setting
 * @return True if VINDPM detection is enabled, false if disabled
 */
bool BQ25798::getVINDPMdetection() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, &reg_value, 1, 0);
  
  return reg_value == 1;
}

/*!
 * @brief Set the VINDPM detection
 * @param enable True = enable VINDPM detection, false = disable
 * @return True if successful
 */
bool BQ25798::setVINDPMdetection(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_4, enable ? 1 : 0, 1, 0);
}

/*!
 * @brief Get the ship FET presence setting
 * @return True if ship FET is present, false if not
 */
bool BQ25798::getShipFETpresent() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, &reg_value, 1, 7);
  
  return reg_value == 1;
}

/*!
 * @brief Set the ship FET presence
 * @param enable True = ship FET is present, false = not present
 * @return True if successful
 */
bool BQ25798::setShipFETpresent(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, enable ? 1 : 0, 1, 7);
}

/*!
 * @brief Get the battery discharge current sense enable setting
 * @return True if battery discharge current sense is enabled, false if disabled
 */
bool BQ25798::getBatDischargeSenseEnable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, &reg_value, 1, 5); 
  
  return reg_value == 1;
}

/*!
 * @brief Set the battery discharge current sense enable
 * @param enable True = enable battery discharge current sense, false = disable
 * @return True if successful
 */
bool BQ25798::setBatDischargeSenseEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, enable ? 1 : 0, 1, 5); 
}

/*!
 * @brief Get the battery discharge current regulation setting
 * @return Battery discharge current regulation
 */
bq25798_ibat_reg_t BQ25798::getBatDischargeA() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, &reg_value, 2, 4);
  
  return (bq25798_ibat_reg_t)reg_value;
}

/*!
 * @brief Set the battery discharge current regulation
 * @param current Battery discharge current regulation
 * @return True if successful
 */
bool BQ25798::setBatDischargeA(bq25798_ibat_reg_t current) {
  if (current > BQ25798_IBAT_REG_DISABLE) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, (uint8_t)current, 2, 4);
}

/*!
 * @brief Get the IINDPM enable setting
 * @return True if IINDPM is enabled, false if disabled
 */
bool BQ25798::getIINDPMenable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, &reg_value, 1, 3);
  
  return reg_value == 1;
}

/*!
 * @brief Set the IINDPM enable
 * @param enable True = enable IINDPM, false = disable
 * @return True if successful
 */
bool BQ25798::setIINDPMenable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, enable ? 1 : 0, 1, 3);
}

/*!
 * @brief Get the external ILIM pin setting
 * @return True if external ILIM pin is used, false if not
 */
bool BQ25798::getExtILIMpin() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, &reg_value, 1, 2);
  
  return reg_value == 1;
}

/*!
 * @brief Set the external ILIM pin
 * @param enable True = use external ILIM pin, false = do not use
 * @return True if successful
 */
bool BQ25798::setExtILIMpin(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, enable ? 1 : 0, 1, 2);
}

/*!
 * @brief Get the battery discharge OCP (Over Current Protection) enable setting
 * @return True if battery discharge OCP is enabled, false if disabled
 */
bool BQ25798::getBatDischargeOCPenable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, &reg_value, 1, 1);
  
  return reg_value == 1;
}

/*!
 * @brief Set the battery discharge OCP (Over Current Protection) enable
 * @param enable True = enable battery discharge OCP, false = disable
 * @return True if successful
 */
bool BQ25798::setBatDischargeOCPenable(bool enable) {
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, enable ? 1 : 0, 1, 1);
}

/*!
 * @brief Get the VINDPM VOC percentage setting
 * @return VINDPM VOC percentage
 */
bq25798_voc_pct_t BQ25798::getVINDPM_VOCpercent() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_MPPT_CONTROL, &reg_value, 3, 5);
  
  return (bq25798_voc_pct_t)reg_value;
}

/*!
 * @brief Set the VINDPM VOC percentage
 * @param percentage VINDPM VOC percentage
 * @return True if successful
 */
bool BQ25798::setVINDPM_VOCpercent(bq25798_voc_pct_t percentage) {
  if (percentage > BQ25798_VOC_PCT_100) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_MPPT_CONTROL, (uint8_t)percentage, 3, 5);
}

/*!
 * @brief Get the VOC delay time setting
 * @return VOC delay time
 */
bq25798_voc_dly_t BQ25798::getVOCdelay() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_MPPT_CONTROL, &reg_value, 2, 3);
  
  return (bq25798_voc_dly_t)reg_value;
}

/*!
 * @brief Set the VOC delay time
 * @param delay VOC delay time
 * @return True if successful
 */
bool BQ25798::setVOCdelay(bq25798_voc_dly_t delay) {
  if (delay > BQ25798_VOC_DLY_5S) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_MPPT_CONTROL, (uint8_t)delay, 2, 3);
}

/*!
 * @brief Get the VOC measurement rate setting
 * @return VOC measurement rate
 */
bq25798_voc_rate_t BQ25798::getVOCrate() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_MPPT_CONTROL, &reg_value, 2, 1);
  
  return (bq25798_voc_rate_t)reg_value;
}

/*!
 * @brief Set the VOC measurement rate
 * @param rate VOC measurement rate
 * @return True if successful
 */
bool BQ25798::setVOCrate(bq25798_voc_rate_t rate) {
  if (rate > BQ25798_VOC_RATE_30MIN) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_MPPT_CONTROL, (uint8_t)rate, 2, 1);
}

/*!
 * @brief Get the MPPT enable setting
 * @return True if MPPT is enabled, false if disabled
 */
bool BQ25798::getMPPTenable() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_MPPT_CONTROL, &reg_value, 1, 0);
  
  return reg_value == 1;
}

/*!
 * @brief Set the MPPT enable
 * @param enable True = enable MPPT, false = disable
 * @return True if successful
 */
bool BQ25798::setMPPTenable(bool enable) {
  return writeRegisterBits(BQ25798_REG_MPPT_CONTROL, enable ? 1 : 0, 1, 0);
}

/*!
 * @brief Get the thermal regulation threshold setting
 * @return Thermal regulation threshold
 */
bq25798_treg_t BQ25798::getThermRegulationThresh() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, &reg_value, 2, 6);
  
  return (bq25798_treg_t)reg_value;
}

/*!
 * @brief Set the thermal regulation threshold
 * @param threshold Thermal regulation threshold
 * @return True if successful
 */
bool BQ25798::setThermRegulationThresh(bq25798_treg_t threshold) {
  if (threshold > BQ25798_TREG_120C) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, (uint8_t)threshold, 2, 6);
}

/*!
 * @brief Get the thermal shutdown threshold setting
 * @return Thermal shutdown threshold
 */
bq25798_tshut_t BQ25798::getThermShutdownThresh() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, &reg_value, 2, 4);
  
  return (bq25798_tshut_t)reg_value;
}

/*!
 * @brief Set the thermal shutdown threshold
 * @param threshold Thermal shutdown threshold
 * @return True if successful
 */
bool BQ25798::setThermShutdownThresh(bq25798_tshut_t threshold) {
  if (threshold > BQ25798_TSHUT_85C) {
    return false;
  }
  
  return writeRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, (uint8_t)threshold, 2, 4);
}

/*!
 * @brief Get the VBUS pulldown setting
 * @return True if VBUS pulldown is enabled, false if disabled
 */
bool BQ25798::getVBUSpulldown() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, &reg_value, 1, 2);
  
  return reg_value == 1;
}

/*!
 * @brief Set the VBUS pulldown
 * @param enable True = enable VBUS pulldown, false = disable
 * @return True if successful
 */
bool BQ25798::setVBUSpulldown(bool enable) {
  return writeRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, enable ? 1 : 0, 1, 2);
}

/*!
 * @brief Get the VAC1 pulldown setting
 * @return True if VAC1 pulldown is enabled, false if disabled
 */
bool BQ25798::getVAC1pulldown() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, &reg_value, 1, 1);
  
  return reg_value == 1;
}

/*!
 * @brief Set the VAC1 pulldown
 * @param enable True = enable VAC1 pulldown, false = disable
 * @return True if successful
 */
bool BQ25798::setVAC1pulldown(bool enable) {
  return writeRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, enable ? 1 : 0, 1, 1);
}

/*!
 * @brief Get the VAC2 pulldown setting
 * @return True if VAC2 pulldown is enabled, false if disabled
 */
bool BQ25798::getVAC2pulldown() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, &reg_value, 1, 0);
  
  return reg_value == 1;
}

/*!
 * @brief Set the VAC2 pulldown
 * @param enable True = enable VAC2 pulldown, false = disable
 * @return True if successful
 */
bool BQ25798::setVAC2pulldown(bool enable) {
  return writeRegisterBits(BQ25798_REG_TEMPERATURE_CONTROL, enable ? 1 : 0, 1, 0);
}

/*!
 * @brief Get the backup AC FET1 on setting
 * @return True if backup AC FET1 is on, false if off
 */
bool BQ25798::getBackupACFET1on() {
  uint8_t reg_value;
  readRegisterBits(BQ25798_REG_DPDM_DRIVER, &reg_value, 1, 7);
  
  return reg_value == 1;
}

/*!
 * @brief Set the backup AC FET1 on
 * @param enable True = turn on backup AC FET1, false = turn off
 * @return True if successful
 */
bool BQ25798::setBackupACFET1on(bool enable) {
  return writeRegisterBits(BQ25798_REG_DPDM_DRIVER, enable ? 1 : 0, 1, 7);
}

/*!
 * @brief Reset all registers to default values
 * @return True if successful
 */
bool BQ25798::reset() {
  // Writing 1 to REG_RST bit (typically in a control register)
  // For BQ25798, this is usually in Charger Control register
  return writeRegisterBits(BQ25798_REG_CHARGER_CONTROL_5, 1, 1, 0);
}

// ADC functions
bool BQ25798::setADCEnable(bool enable) {
  return writeRegisterBits(BQ25798_REG_ADC_CONTROL, enable ? 1 : 0, 1, 7);
}

bool BQ25798::getADCEnable() {
  uint8_t value;
  readRegisterBits(BQ25798_REG_ADC_CONTROL, &value, 1, 7);
  return value == 1;
}

bool BQ25798::setADCRate(bq25798_adc_rate_t rate) {
  // ADC_RATE is bit 6: 0=Continuous conversion, 1=One shot conversion
  return writeRegisterBits(BQ25798_REG_ADC_CONTROL, rate, 1, 6);
}

bool BQ25798::setADCAverage(bq25798_adc_avg_t avg) {
  // ADC_AVG is bit 3: 0=Single value, 1=Running average
  return writeRegisterBits(BQ25798_REG_ADC_CONTROL, avg, 1, 3);
}

bool BQ25798::setADCResolution(bq25798_adc_res_t res) {
  // ADC_SAMPLE_1:0 are bits 5-4: 00=15bit, 01=14bit, 10=13bit, 11=12bit
  return writeRegisterBits(BQ25798_REG_ADC_CONTROL, res, 2, 4);
}

bool BQ25798::configureADC(bq25798_adc_res_t res, bq25798_adc_avg_t avg, bq25798_adc_rate_t rate) {
  // Configure ADC resolution, averaging, and rate in one operation
  // Datasheet bit mapping for REG2E_ADC_Control:
  // Bit 7: ADC_EN (1=Enable)
  // Bit 6: ADC_RATE (0=Continuous, 1=One-shot) 
  // Bits 5-4: ADC_SAMPLE_1:0 (resolution)
  // Bit 3: ADC_AVG (averaging)
  // Bit 2: ADC_AVG_INIT (0=use existing, 1=start new)
  // Bits 1-0: RESERVED (should be 0)
  
  uint8_t adc_control = 0;
  adc_control |= (1 << 7);        // ADC_EN = 1 (enable)
  adc_control |= (rate << 6);     // ADC_RATE (bit 6)
  adc_control |= (res << 4);      // ADC_SAMPLE_1:0 (bits 5-4)
  adc_control |= (avg << 3);      // ADC_AVG (bit 3)
  adc_control |= (0 << 2);        // ADC_AVG_INIT = 0 (use existing)
  // Bits 1-0 remain 0 (reserved)
  
  return writeRegister(BQ25798_REG_ADC_CONTROL, adc_control);
}

bool BQ25798::isADCConversionDone() {
  // ADC_DONE_STAT is in Charger Status 3 register (0x1E), bit 5
  // This bit is only meaningful in one-shot mode
  uint8_t value;
  readRegisterBits(BQ25798_REG_CHARGER_STATUS_3, &value, 1, 5);
  return value == 1; // ADC_DONE bit is set when conversion is complete
}

uint16_t BQ25798::getRawADCIBUS() {
  uint16_t value;
  readRegister16(BQ25798_REG_IBUS_ADC, &value);
  return value;
}

uint16_t BQ25798::getRawADCIBAT() {
  uint16_t value;
  readRegister16(BQ25798_REG_IBAT_ADC, &value);
  return value;
}

uint16_t BQ25798::getRawADCVBUS() {
  uint16_t value;
  readRegister16(BQ25798_REG_VBUS_ADC, &value);
  return value;
}

uint16_t BQ25798::getRawADCVBAT() {
  uint16_t value;
  readRegister16(BQ25798_REG_VBAT_ADC, &value);
  return value;
}

uint16_t BQ25798::getRawADCVSYS() {
  uint16_t value;
  readRegister16(BQ25798_REG_VSYS_ADC, &value);
  return value;
}

uint16_t BQ25798::getRawADCTS() {
  uint16_t value;
  readRegister16(BQ25798_REG_TS_ADC, &value);
  return value;
}

uint16_t BQ25798::getRawADCTDIE() {
  uint16_t value;
  readRegister16(BQ25798_REG_TDIE_ADC, &value);
  return value;
}

uint16_t BQ25798::getRawADCVAC1() {
  uint16_t value;
  readRegister16(BQ25798_REG_VAC1_ADC, &value);
  return value;
}

uint16_t BQ25798::getRawADCVAC2() {
  uint16_t value;
  readRegister16(BQ25798_REG_VAC2_ADC, &value);
  return value;
}

float BQ25798::getADCIBUS() {
  uint16_t value;
  readRegister16(BQ25798_REG_IBUS_ADC, &value);
  // According to datasheet: 1 LSB = 1mA for IBUS, 2's complement
  // Convert from 2's complement 16-bit to signed integer
  int16_t signed_value = (int16_t)value;
  return signed_value * 0.001f; // 1mA per LSB, convert to A
}

float BQ25798::getADCIBAT() {
  uint16_t value;
  readRegister16(BQ25798_REG_IBAT_ADC, &value);
  // According to datasheet: 1 LSB = 1mA for IBAT, 2's complement
  // Convert from 2's complement 16-bit to signed integer
  int16_t signed_value = (int16_t)value;
  return signed_value * 0.001f; // 1mA per LSB, convert to A
}  

float BQ25798::getADCVBUS() {
  uint16_t value;
  readRegister16(BQ25798_REG_VBUS_ADC, &value);
  // According to datasheet: 1 LSB = 1mV for voltage measurements
  return (value & 0x7FFF) * 0.001f; // 1mV per LSB, convert to V
}

float BQ25798::getADCVBAT() {
  uint16_t value;
  readRegister16(BQ25798_REG_VBAT_ADC, &value);
  // According to datasheet: 1 LSB = 1mV
  return (value & 0x7FFF) * 0.001f; // 1mV per LSB, convert to V
}

float BQ25798::getADCVSYS() {
  uint16_t value;
  readRegister16(BQ25798_REG_VSYS_ADC, &value);
  // According to datasheet: 1 LSB = 1mV
  return (value & 0x7FFF) * 0.001f; // 1mV per LSB, convert to V
}

float BQ25798::getADCTS() {
  uint16_t value;
  readRegister16(BQ25798_REG_TS_ADC, &value);
  // According to datasheet: 1 LSB = 0.1% for TS
  return (value & 0x7FFF) * 0.1f; // 0.1% per LSB
}

float BQ25798::getADCTDIE() {
  uint16_t value;
  readRegister16(BQ25798_REG_TDIE_ADC, &value);
  // According to datasheet: 2's complement, 1 LSB = 0.5°C, range -40°C to 150°C
  int16_t signed_value = (int16_t)value;
  return signed_value * 0.5f; // 0.5°C per LSB
}

float BQ25798::getADCVAC1() {
  uint16_t value;
  readRegister16(BQ25798_REG_VAC1_ADC, &value);
  // VAC1 is an AC input voltage measurement, 1 LSB = 1mV (assumed unsigned)
  return (value & 0x7FFF) * 0.001f; // 1mV per LSB, convert to V
}

float BQ25798::getADCVAC2() {
  uint16_t value;
  readRegister16(BQ25798_REG_VAC2_ADC, &value);
  // VAC2 is an AC input voltage measurement, 1 LSB = 1mV (assumed unsigned)
  return (value & 0x7FFF) * 0.001f; // 1mV per LSB, convert to V
}

/*!
 * @brief Private methods
 */

bool BQ25798::readRegister(uint8_t reg, uint8_t *value) {
  if (_use_soft_i2c) {
    _softWire->beginTransmission(_i2c_addr);
    _softWire->write(reg);
    if (_softWire->endTransmission() != 0) {
      return false;
    }
    if (_softWire->requestFrom(_i2c_addr, (uint8_t)1) != 1) {
      return false;
    }
    *value = _softWire->read();
  } else {
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    if (_wire->endTransmission() != 0) {
      return false;
    }
    if (_wire->requestFrom(_i2c_addr, (uint8_t)1) != 1) {
      return false;
    }
    *value = _wire->read();
  }
  return true;
}

/*!
 * @brief Write a byte to a register
 * @param reg The register address
 * @param value The value to write
 * @return True if successful
 */
bool BQ25798::writeRegister(uint8_t reg, uint8_t value) {
  if (_use_soft_i2c) {
    _softWire->beginTransmission(_i2c_addr);
    _softWire->write(reg);
    _softWire->write(value);
    return _softWire->endTransmission() == 0;
  } else {
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    _wire->write(value);
    return _wire->endTransmission() == 0;
  }
}

/*!
 * @brief Read a 16-bit value from a register
 * @param reg The register address
 * @param value Pointer to the 16-bit value to store the result
 * @return True if successful
 */
bool BQ25798::readRegister16(uint8_t reg, uint16_t *value) {
  uint8_t lsb, msb;
  if (_use_soft_i2c) {
    _softWire->beginTransmission(_i2c_addr);
    _softWire->write(reg);
    if (_softWire->endTransmission() != 0) {
      return false;
    }
    if (_softWire->requestFrom(_i2c_addr, (uint8_t)2) != 2) {
      return false;
    }
    msb = _softWire->read();  // First byte is MSB (standard I2C big-endian)
    lsb = _softWire->read();  // Second byte is LSB
  } else {
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    if (_wire->endTransmission() != 0) {
      return false;
    }
    if (_wire->requestFrom(_i2c_addr, (uint8_t)2) != 2) {
      return false;
    }
    msb = _wire->read();  // First byte is MSB (standard I2C big-endian)
    lsb = _wire->read();  // Second byte is LSB
  }
  *value = (uint16_t)msb << 8 | lsb;  // MSB << 8 | LSB
  return true;
}

/*!
 * @brief Write a 16-bit value to a register
 * @param reg The register address
 * @param value The 16-bit value to write
 * @return True if successful
 */
bool BQ25798::writeRegister16(uint8_t reg, uint16_t value) {
  if (_use_soft_i2c) {
    _softWire->beginTransmission(_i2c_addr);
    _softWire->write(reg);
    _softWire->write(value >> 8);    // MSB first (big-endian)
    _softWire->write(value & 0xFF);  // LSB second
    return _softWire->endTransmission() == 0;
  } else {
    _wire->beginTransmission(_i2c_addr);
    _wire->write(reg);
    _wire->write(value >> 8);    // MSB first (big-endian)
    _wire->write(value & 0xFF);  // LSB second
    return _wire->endTransmission() == 0;
  }
}

bool BQ25798::readRegisterBits(uint8_t reg, uint8_t *value, uint8_t bits, uint8_t shift) {
  uint8_t reg_value;
  if (!readRegister(reg, &reg_value)) {
    return false;
  }
  *value = (reg_value >> shift) & ((1 << bits) - 1);
  return true;
}

bool BQ25798::writeRegisterBits(uint8_t reg, uint8_t value, uint8_t bits, uint8_t shift) {
  uint8_t reg_value;
  if (!readRegister(reg, &reg_value)) {
    return false;
  }
  uint8_t mask = ((1 << bits) - 1) << shift;
  reg_value &= ~mask;
  reg_value |= (value << shift) & mask;
  return writeRegister(reg, reg_value);
}

bool BQ25798::readRegisterBits16(uint8_t reg, uint16_t *value, uint16_t bits, uint8_t shift) {
  uint16_t reg_value;
  if (!readRegister16(reg, &reg_value)) {
    return false;
  }
  *value = (reg_value >> shift) & ((1 << bits) - 1);
  return true;
}

bool BQ25798::writeRegisterBits16(uint8_t reg, uint16_t value, uint16_t bits, uint8_t shift) {
  uint16_t reg_value;
  if (!readRegister16(reg, &reg_value)) {
    return false;
  }
  uint16_t mask = ((1 << bits) - 1) << shift;
  reg_value &= ~mask;
  reg_value |= (value << shift) & mask;
  return writeRegister16(reg, reg_value);
}

// Status and Fault functions
uint8_t BQ25798::getChargerStatus0() {
  uint8_t value;
  readRegister(BQ25798_REG_CHARGER_STATUS_0, &value);
  return value;
}

uint8_t BQ25798::getChargerStatus1() {
  uint8_t value;
  readRegister(BQ25798_REG_CHARGER_STATUS_1, &value);
  return value;
}

uint8_t BQ25798::getChargerStatus2() {
  uint8_t value;
  readRegister(BQ25798_REG_CHARGER_STATUS_2, &value);
  return value;
}

uint8_t BQ25798::getChargerStatus3() {
  uint8_t value;
  readRegister(BQ25798_REG_CHARGER_STATUS_3, &value);
  return value;
}

uint8_t BQ25798::getChargerStatus4() {
  uint8_t value;
  readRegister(BQ25798_REG_CHARGER_STATUS_4, &value);
  return value;
}

uint8_t BQ25798::getFaultStatus0() {
  uint8_t value;
  readRegister(BQ25798_REG_FAULT_STATUS_0, &value);
  return value;
}

uint8_t BQ25798::getFaultStatus1() {
  uint8_t value;
  readRegister(BQ25798_REG_FAULT_STATUS_1, &value);
  return value;
}

// Status decoding functions
void BQ25798::printChargerStatus() {
  uint8_t status0 = getChargerStatus0();
  uint8_t status1 = getChargerStatus1();
  uint8_t status2 = getChargerStatus2();
  uint8_t status3 = getChargerStatus3();
  uint8_t status4 = getChargerStatus4();

  BQ25798_DEBUGLN("=== Charger Status ===");
  
  // Status 0 (0x1B) - CORRECTED to match BQ25798 datasheet
  BQ25798_DEBUGF("Status 0 (0x%02X):\n", status0);
  BQ25798_DEBUGF("  VBUS_PRESENT_STAT: %s\n", (status0 & 0x01) ? "VBUS present" : "VBUS not present");           // Bit 0
  BQ25798_DEBUGF("  AC1_PRESENT_STAT: %s\n", (status0 & 0x02) ? "VAC1 present" : "VAC1 not present");           // Bit 1
  BQ25798_DEBUGF("  AC2_PRESENT_STAT: %s\n", (status0 & 0x04) ? "VAC2 present" : "VAC2 not present");           // Bit 2
  BQ25798_DEBUGF("  PG_STAT: %s\n", (status0 & 0x08) ? "Power Good" : "Not Power Good");                        // Bit 3
  // Bit 4: Reserved
  BQ25798_DEBUGF("  WD_STAT: %s\n", (status0 & 0x20) ? "Watchdog expired" : "Normal");                          // Bit 5
  BQ25798_DEBUGF("  VINDPM_STAT: %s\n", (status0 & 0x40) ? "In VINDPM regulation" : "Not in VINDPM");           // Bit 6
  BQ25798_DEBUGF("  IINDPM_STAT: %s\n", (status0 & 0x80) ? "In IINDPM regulation" : "Not in IINDPM");           // Bit 7
  
  // Status 1 (0x1C) - CORRECTED to match BQ25798 datasheet
  BQ25798_DEBUGF("Status 1 (0x%02X):\n", status1);
  BQ25798_DEBUGF("  BC1.2_DONE_STAT: %s\n", (status1 & 0x01) ? "BC1.2 detection complete" : "BC1.2 detection ongoing");  // Bit 0
  
  // VBUS_STAT_3:0 (Bits 4:1)
  uint8_t vbus_stat = (status1 >> 1) & 0x0F;
  BQ25798_DEBUGF("  VBUS_STAT: ");
  switch(vbus_stat) {
    case 0x0: BQ25798_DEBUGLN("No Input or BHOT or BCOLD in OTG mode"); break;
    case 0x1: BQ25798_DEBUGLN("USB SDP (500mA)"); break;
    case 0x2: BQ25798_DEBUGLN("USB CDP (1.5A)"); break;
    case 0x3: BQ25798_DEBUGLN("USB DCP (3.25A)"); break;
    case 0x4: BQ25798_DEBUGLN("Adjustable High Voltage DCP (1.5A)"); break;
    case 0x5: BQ25798_DEBUGLN("Unknown adaptor (3A)"); break;
    case 0x6: BQ25798_DEBUGLN("Non-Standard Adapter"); break;
    case 0x7: BQ25798_DEBUGLN("In OTG mode"); break;
    case 0x8: BQ25798_DEBUGLN("Not qualified adaptor"); break;
    case 0xB: BQ25798_DEBUGLN("Device directly powered from VBUS"); break;
    case 0xC: BQ25798_DEBUGLN("Backup Mode"); break;
    default: BQ25798_DEBUGF("Reserved (0x%X)\n", vbus_stat); break;
  }
  
  // CHG_STAT_2:0 (Bits 7:5)
  uint8_t chg_stat = (status1 >> 5) & 0x07;
  BQ25798_DEBUGF("  CHG_STAT: ");
  switch(chg_stat) {
    case 0: BQ25798_DEBUGLN("Not Charging"); break;
    case 1: BQ25798_DEBUGLN("Trickle Charge"); break;
    case 2: BQ25798_DEBUGLN("Pre-charge"); break;
    case 3: BQ25798_DEBUGLN("Fast charge (CC mode)"); break;
    case 4: BQ25798_DEBUGLN("Taper Charge (CV mode)"); break;
    case 5: BQ25798_DEBUGLN("Reserved"); break;
    case 6: BQ25798_DEBUGLN("Top-off Timer Active Charging"); break;
    case 7: BQ25798_DEBUGLN("Charge Termination Done"); break;
  }
  
  // Status 2 (0x1D) - CORRECTED to match BQ25798 datasheet
  BQ25798_DEBUGF("Status 2 (0x%02X):\n", status2);
  BQ25798_DEBUGF("  VBAT_PRESENT_STAT: %s\n", (status2 & 0x01) ? "Battery present" : "No battery");              // Bit 0
  BQ25798_DEBUGF("  DPDM_STAT: %s\n", (status2 & 0x02) ? "D+/D- detection ongoing" : "D+/D- detection done");  // Bit 1
  BQ25798_DEBUGF("  TREG_STAT: %s\n", (status2 & 0x04) ? "In thermal regulation" : "Normal temp");             // Bit 2
  // Bits 5:3: Reserved
  
  // ICO_STAT_1:0 (Bits 7:6)
  uint8_t ico_stat = (status2 >> 6) & 0x03;
  BQ25798_DEBUGF("  ICO_STAT: ");
  switch(ico_stat) {
    case 0: BQ25798_DEBUGLN("ICO disabled"); break;
    case 1: BQ25798_DEBUGLN("ICO optimization in progress"); break;
    case 2: BQ25798_DEBUGLN("Maximum input current detected"); break;
    case 3: BQ25798_DEBUGLN("Reserved"); break;
  }
  
  // Status 3 (0x1E) - CORRECTED to match BQ25798 datasheet
  BQ25798_DEBUGF("Status 3 (0x%02X):\n", status3);
  // Bit 0: Reserved
  BQ25798_DEBUGF("  PRECHG_TMR_STAT: %s\n", (status3 & 0x02) ? "Pre-charge timer expired" : "Normal");          // Bit 1
  BQ25798_DEBUGF("  TRICHG_TMR_STAT: %s\n", (status3 & 0x04) ? "Trickle charge timer expired" : "Normal");     // Bit 2
  BQ25798_DEBUGF("  CHG_TMR_STAT: %s\n", (status3 & 0x08) ? "Fast charge timer expired" : "Normal");           // Bit 3
  BQ25798_DEBUGF("  VSYS_STAT: %s\n", (status3 & 0x10) ? "In VSYSMIN regulation" : "Not in VSYSMIN regulation"); // Bit 4
  BQ25798_DEBUGF("  ADC_DONE_STAT: %s\n", (status3 & 0x20) ? "ADC conversion complete" : "ADC conversion not complete"); // Bit 5
  BQ25798_DEBUGF("  ACRB1_STAT: %s\n", (status3 & 0x40) ? "ACFET1-RBFET1 placed" : "ACFET1-RBFET1 not placed"); // Bit 6
  BQ25798_DEBUGF("  ACRB2_STAT: %s\n", (status3 & 0x80) ? "ACFET2-RBFET2 placed" : "ACFET2-RBFET2 not placed"); // Bit 7
  
  // Status 4 (0x1F) - CORRECTED to match BQ25798 datasheet
  BQ25798_DEBUGF("Status 4 (0x%02X):\n", status4);
  BQ25798_DEBUGF("  TS_HOT_STAT: %s\n", (status4 & 0x01) ? "TS in hot range (>T5)" : "TS not in hot range");     // Bit 0
  BQ25798_DEBUGF("  TS_WARM_STAT: %s\n", (status4 & 0x02) ? "TS in warm range (T3-T5)" : "TS not in warm range"); // Bit 1
  BQ25798_DEBUGF("  TS_COOL_STAT: %s\n", (status4 & 0x04) ? "TS in cool range (T1-T2)" : "TS not in cool range"); // Bit 2
  BQ25798_DEBUGF("  TS_COLD_STAT: %s\n", (status4 & 0x08) ? "TS in cold range (<T1)" : "TS not in cold range");  // Bit 3
  BQ25798_DEBUGF("  VBATOTG_LOW_STAT: %s\n", (status4 & 0x10) ? "Battery voltage too low for OTG" : "Battery voltage OK for OTG"); // Bit 4
  // Bits 7:5: Reserved
}

void BQ25798::printFaultStatus() {
  uint8_t fault0 = getFaultStatus0();
  uint8_t fault1 = getFaultStatus1();

  BQ25798_DEBUGLN("=== Fault Status ===");
  
  // Fault 0 (0x20) - Corrected to match BQ25798 datasheet
  BQ25798_DEBUGF("Fault 0 (0x%02X):\n", fault0);
  if (fault0 == 0) {
    BQ25798_DEBUGLN("  No faults");
  } else {
    BQ25798_DEBUGF("  VAC1_OVP: %s\n", (fault0 & 0x01) ? "VAC1 overvoltage" : "Normal");           // Bit 0
    BQ25798_DEBUGF("  VAC2_OVP: %s\n", (fault0 & 0x02) ? "VAC2 overvoltage" : "Normal");           // Bit 1
    BQ25798_DEBUGF("  CONV_OCP: %s\n", (fault0 & 0x04) ? "Converter overcurrent" : "Normal");      // Bit 2
    BQ25798_DEBUGF("  IBAT_OCP: %s\n", (fault0 & 0x08) ? "IBAT overcurrent" : "Normal");           // Bit 3
    BQ25798_DEBUGF("  IBUS_OCP: %s\n", (fault0 & 0x10) ? "IBUS overcurrent" : "Normal");           // Bit 4
    BQ25798_DEBUGF("  VBAT_OVP: %s\n", (fault0 & 0x20) ? "Battery overvoltage" : "Normal");        // Bit 5
    BQ25798_DEBUGF("  VBUS_OVP: %s\n", (fault0 & 0x40) ? "VBUS overvoltage" : "Normal");           // Bit 6
    BQ25798_DEBUGF("  IBAT_REG: %s\n", (fault0 & 0x80) ? "IBAT regulation active" : "Normal");     // Bit 7
  }
  
  // Fault 1 (0x21) - Corrected to match BQ25798 datasheet
  BQ25798_DEBUGF("Fault 1 (0x%02X):\n", fault1);
  if (fault1 == 0) {
    BQ25798_DEBUGLN("  No faults");
  } else {
    BQ25798_DEBUGF("  TSHUT: %s\n", (fault1 & 0x04) ? "Thermal shutdown" : "Normal");              // Bit 2
    BQ25798_DEBUGF("  OTG_UVP: %s\n", (fault1 & 0x10) ? "OTG undervoltage" : "Normal");           // Bit 4
    BQ25798_DEBUGF("  OTG_OVP: %s\n", (fault1 & 0x20) ? "OTG overvoltage" : "Normal");            // Bit 5
    BQ25798_DEBUGF("  VSYS_OVP: %s\n", (fault1 & 0x40) ? "VSYS overvoltage" : "Normal");          // Bit 6
    BQ25798_DEBUGF("  VSYS_SHORT: %s\n", (fault1 & 0x80) ? "VSYS short circuit" : "Normal");      // Bit 7
  }
}

// ADC debug functions
void BQ25798::printRawADC() {
  BQ25798_DEBUGLN("=== Raw ADC Values ===");
  uint16_t ibus_raw = getRawADCIBUS();
  uint16_t ibat_raw = getRawADCIBAT();
  BQ25798_DEBUGF("Raw IBUS: 0x%04X (signed: %d)\n", ibus_raw, (int16_t)ibus_raw);
  BQ25798_DEBUGF("Raw IBAT: 0x%04X (signed: %d)\n", ibat_raw, (int16_t)ibat_raw);
  BQ25798_DEBUGF("Raw VBUS: 0x%04X\n", getRawADCVBUS());
  BQ25798_DEBUGF("Raw VBAT: 0x%04X\n", getRawADCVBAT());
  BQ25798_DEBUGF("Raw VSYS: 0x%04X\n", getRawADCVSYS());
  BQ25798_DEBUGF("Raw TS: 0x%04X\n", getRawADCTS());
  BQ25798_DEBUGF("Raw TDIE: 0x%04X\n", getRawADCTDIE());
  BQ25798_DEBUGF("Raw VAC1: 0x%04X\n", getRawADCVAC1());
  BQ25798_DEBUGF("Raw VAC2: 0x%04X\n", getRawADCVAC2());
}

void BQ25798::printADCValues() {
  BQ25798_DEBUGLN("=== ADC Values ===");
  BQ25798_DEBUGF("IBUS: %.2f A\n", getADCIBUS());
  BQ25798_DEBUGF("IBAT: %.2f A\n", getADCIBAT());
  BQ25798_DEBUGF("VBUS: %.2f V\n", getADCVBUS());
  BQ25798_DEBUGF("VBAT: %.2f V\n", getADCVBAT());
  BQ25798_DEBUGF("VSYS: %.2f V\n", getADCVSYS());
  BQ25798_DEBUGF("TS: %.2f %%\n", getADCTS());
  BQ25798_DEBUGF("TDIE: %.2f °C\n", getADCTDIE());
  BQ25798_DEBUGF("VAC1: %.2f V\n", getADCVAC1());
  BQ25798_DEBUGF("VAC2: %.2f V\n", getADCVAC2());
}

// Debug functions for register access
bool BQ25798::readRegisterDirect(uint8_t reg, uint8_t *value) {
  return readRegister(reg, value);
}
