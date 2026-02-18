/*!
 * @file ModbeeMpptDebug.cpp
 * 
 * @brief void ModbeeMpptDebug::printPowerMeasurements() {
  printSectionHeader("POWER MEASUREMENTS");
  
  // Create one API instance for this function
  ModbeeMpptAPI api(_mppt);
  
  // Get power data using the API
  modbee_power_data_t vbus = api.getVbusPower();
  modbee_power_data_t battery = api.getBatteryPower();
  modbee_power_data_t system = api.getSystemPower();
  modbee_power_data_t vac1 = api.getVAC1Power();
  modbee_power_data_t vac2 = api.getVAC2Power();tion of debug and diagnostic functions for ModbeeMPPT
 */

#include <cmath>
#include "ModbeeMpptDebug.h"
#include "ModbeeMPPT.h"
#include "ModbeeMpptAPI.h"

ModbeeMpptDebug::ModbeeMpptDebug(ModbeeMPPT& mppt) : _mppt(mppt) {
  // No need to create API - use the one from MPPT
}

ModbeeMpptDebug::~ModbeeMpptDebug() {
  // Nothing to clean up
}

// ========================================================================
// COMPREHENSIVE DEBUG OUTPUT
// ========================================================================

void ModbeeMpptDebug::printCompleteStatus() {
  
  printPowerMeasurements();
  
  printComprehensiveBatteryStatus();
  
  printConfiguration();
  
  printStatus();
  
  printFaults();

  printPowerPathDiagnostics();

  printRawRegisters();

  printRegisterDecoding();
  
}

// ========================================================================
// INDIVIDUAL SECTION PRINTING
// ========================================================================

void ModbeeMpptDebug::printPowerMeasurements() {
  printSectionHeader("POWER MEASUREMENTS", 80);
  
  // Get power data using the single API instance
  modbee_power_data_t vbus = _mppt.api.getVbusPower();
  modbee_power_data_t battery = _mppt.api.getBatteryPower();
  modbee_power_data_t system = _mppt.api.getSystemPower();
  modbee_power_data_t vac1 = _mppt.api.getVAC1Power();
  modbee_power_data_t vac2 = _mppt.api.getVAC2Power();
  
  // Input sources
  printSubsectionHeader("Input Sources");
  Serial.println(formatField("VBUS Voltage:", String(vbus.voltage, 2) + "V"));
  Serial.println(formatField("VBUS Current:", String(vbus.current, 3) + "A"));
  Serial.println(formatField("VBUS Power:", String(vbus.power, 2) + "W"));
  Serial.println(formatField("VAC1 Voltage:", String(vac1.voltage, 2) + "V"));
  Serial.println(formatField("VAC2 Voltage:", String(vac2.voltage, 2) + "V"));
  
  // Battery
  printSubsectionHeader("Battery Status");
  Serial.println(formatField("Battery Voltage:", String(battery.voltage, 2) + "V"));
  Serial.println(formatField("Battery Current:", String(battery.current, 3) + "A (" + _mppt.api.getBatteryCurrentDirection() + ")"));
  Serial.println(formatField("Battery Power:", String(battery.power, 2) + "W"));
  Serial.println(formatField("Battery SOC:", String(_mppt.api.getBatteryChargePercent(), 1) + "%"));
  
  // System
  printSubsectionHeader("System Load");
  Serial.println(formatField("System Voltage:", String(system.voltage, 2) + "V"));
  Serial.println(formatField("System Current:", String(system.current, 3) + "A"));
  Serial.println(formatField("System Power:", String(system.power, 2) + "W"));
  
  // Efficiency & Temperature
  printSubsectionHeader("Performance");
  Serial.println(formatField("Conversion Efficiency:", String(_mppt.api.getEfficiency(), 1) + "%"));
  Serial.println(formatField("Die Temperature:", String(_mppt.api.getDieTemperature(), 1) + " degC"));
  Serial.println(formatField("Battery Temperature:", String(_mppt.api.getBatteryTemperature(), 1) + " degC"));
  Serial.println(formatField("TS ADC Raw (%):", String(_mppt.api.getRawTSPercent(), 1) + "%"));
}

void ModbeeMpptDebug::printConfiguration() {
  printSectionHeader("CONFIGURATION SETTINGS", 80);
  
  // Charging configuration
  printSubsectionHeader("Charging Configuration");
  Serial.println(formatField("Charge Enable:", formatStatusClean(_mppt.api.getChargeEnable())));
  Serial.println(formatField("Charge Voltage:", String(_mppt.api.getChargeVoltage(), 2) + "V"));
  Serial.println(formatField("Charge Current:", String(_mppt.api.getChargeCurrent(), 2) + "A"));
  Serial.println(formatField("Termination Current:", String(_mppt.api.getTerminationCurrent() * 1000, 0) + "mA"));
  Serial.println(formatField("Recharge Threshold:", String(_mppt.api.getRechargeThreshold() * 1000, 0) + "mV"));
  Serial.println(formatField("Precharge Current:", String(_mppt.api.getPrechargeCurrent() * 1000, 0) + "mA"));
  
  // Precharge voltage threshold with readable string
  modbee_vbat_lowv_t prechargeThreshold = _mppt.api.getPrechargeVoltageThreshold();
  String prechargeThreshStr;
  switch(prechargeThreshold) {
    case MODBEE_VBAT_LOWV_15_PERCENT: prechargeThreshStr = "15% of VREG"; break;
    case MODBEE_VBAT_LOWV_62_2_PERCENT: prechargeThreshStr = "62.2% of VREG"; break;
    case MODBEE_VBAT_LOWV_66_7_PERCENT: prechargeThreshStr = "66.7% of VREG"; break;
    case MODBEE_VBAT_LOWV_71_4_PERCENT: prechargeThreshStr = "71.4% of VREG"; break;
    default: prechargeThreshStr = "Unknown"; break;
  }
  Serial.println(formatField("Precharge→Fast Threshold:", prechargeThreshStr));
  Serial.println(formatField("Min System Voltage:", String(_mppt.api.getMinSystemVoltage(), 1) + "V"));
  
  // Input limits
  printSubsectionHeader("Input Limits & Protection");
  Serial.println(formatField("Input Voltage Limit:", String(_mppt.api.getInputVoltageLimit(), 1) + "V"));
  Serial.println(formatField("Input Current Limit:", String(_mppt.api.getInputCurrentLimit(), 2) + "A"));
  Serial.println(formatField("VAC OVP Threshold:", String(_mppt.api.getVACOVP(), 1) + "V"));
  
  // Protection settings
  printSubsectionHeader("Protection Settings");
  Serial.println(formatField("Watchdog Enable:", formatStatusClean(_mppt.api.getWatchdogEnable())));
  Serial.println(formatField("Cell Count:", String(_mppt.api.getCellCount())));
  Serial.println(formatField("Max Charge Current:", String(_mppt.api.getChargeCurrent(), 1) + "A"));
  
  // Timers
  printSubsectionHeader("Timer Configuration");
  Serial.println(formatField("Fast Charge Timer:", formatStatusClean(_mppt.api.getFastChargeTimerEnable())));
  Serial.println(formatField("Precharge Timer:", formatStatusClean(_mppt.api.getPrechargeTimerEnable())));
  Serial.println(formatField("Trickle Charge Timer:", formatStatusClean(_mppt.api.getTrickleChargeTimerEnable())));
  Serial.println(formatField("Timer Half Rate:", formatStatusClean(_mppt.api.getTimerHalfRateEnable())));
  
  // Top-off timer with readable string
  modbee_topoff_timer_t topoffTimer = _mppt.api.getTopOffTimer();
  String topoffStr;
  switch(topoffTimer) {
    case MODBEE_TOPOFF_TIMER_DISABLED: topoffStr = "Disabled"; break;
    case MODBEE_TOPOFF_TIMER_15MIN: topoffStr = "15 minutes"; break;
    case MODBEE_TOPOFF_TIMER_30MIN: topoffStr = "30 minutes"; break;
    case MODBEE_TOPOFF_TIMER_45MIN: topoffStr = "45 minutes"; break;
    default: topoffStr = "Unknown"; break;
  }
  Serial.println(formatField("Top-off Timer:", topoffStr));
  
  // ADC & MPPT configuration
  printSubsectionHeader("Control Systems");
  Serial.println(formatField("ADC Enable:", formatStatusClean(_mppt.api.getADCEnable())));
  Serial.println(formatField("MPPT Enable:", formatStatusClean(_mppt.api.getMPPTEnable())));
  Serial.println(formatField("VOC Percentage:", String(_mppt.api.vocPercentToFloat(_mppt.api.getMPPTVOCPercent()), 2) + "% (enum: " + String(_mppt.api.getMPPTVOCPercent()) + ")"));
}

void ModbeeMpptDebug::printStatus() {
  printSectionHeader("SYSTEM STATUS", 80);
  
  // Main status
  printSubsectionHeader("Charging Status");
  Serial.println(formatField("Charge State:", _mppt.api.getChargeStateString()));
  Serial.println(formatField("Power Good:", formatStatusClean(_mppt.api.isPowerGood())));
  Serial.println(formatField("Battery Present:", formatStatusClean(_mppt.api.isBatteryPresent())));
  Serial.println(formatField("VBUS Present:", formatStatusClean(_mppt.api.isVbusPresent())));
  
  // Power management status
  printSubsectionHeader("Power Management");
  Serial.println(formatField("Input Power Management:", formatStatusClean(_mppt.api.isInInputPowerManagement())));
  Serial.println(formatField("Thermal Regulation:", formatStatusClean(_mppt.api.isInThermalRegulation())));
  Serial.println(formatField("ADC Conversion Done:", formatStatusClean(_mppt.api.isADCComplete())));
  Serial.println(formatField("ICO Complete:", formatStatusClean(_mppt.api.isICOComplete())));
  
  // Detailed status registers
  printSubsectionHeader("Status Register Details");
  Serial.println(formatField("Status 0:", _mppt.api.getStatus0String()));
  Serial.println(formatField("Status 1:", _mppt.api.getStatus1String()));
  Serial.println(formatField("Status 2:", _mppt.api.getStatus2String()));
  Serial.println(formatField("Status 3:", _mppt.api.getStatus3String()));
  Serial.println(formatField("Status 4:", _mppt.api.getStatus4String()));
}

void ModbeeMpptDebug::printFaults() {
  printSectionHeader("FAULT STATUS", 80);
  
  bool hasFaults = _mppt.api.hasFaults();
  
  if (hasFaults) {
    Serial.println(formatField("Overall Status:", "FAULTS DETECTED"));
    Serial.println(formatField("Active Faults:", _mppt.api.getFaultString()));
    
    // Specific fault categories
    printSubsectionHeader("Fault Categories");
    Serial.println(formatField("Input Overvoltage:", formatStatusClean(_mppt.api.hasInputOvervoltageFault())));
    Serial.println(formatField("Overcurrent:", formatStatusClean(_mppt.api.hasOvercurrentFault())));
    Serial.println(formatField("Thermal:", formatStatusClean(_mppt.api.hasThermalFault())));
  } else {
    Serial.println(formatField("Overall Status:", "NO FAULTS DETECTED"));
    Serial.println(formatField("System Status:", "All systems operating normally"));
  }
}

void ModbeeMpptDebug::printPowerPathDiagnostics() {
  printSectionHeader("POWER PATH DIAGNOSTICS", 80);
  
  // Get status for power path analysis
  modbee_status3_t status3 = _mppt.api.getStatus3();
  modbee_fault0_t fault0 = _mppt.api.getFault0();
  
  // Power path FET status
  printSubsectionHeader("Power Path FET Status");
  Serial.println(formatField("ACRB1 Active:", formatStatusClean(status3.acrb1_active)));
  Serial.println(formatField("ACRB2 Active:", formatStatusClean(status3.acrb2_active)));
  
  // Power routing analysis
  printSubsectionHeader("Power Flow Analysis");
  float vbat = _mppt.api.getBatteryVoltage();
  float vsys = _mppt.api.getSystemVoltage();
  float ibat = _mppt.api.getBatteryCurrent();
  
  Serial.println(formatField("VBAT:", String(vbat, 2) + "V"));
  Serial.println(formatField("VSYS:", String(vsys, 2) + "V"));
  Serial.println(formatField("IBAT:", String(ibat, 3) + "A"));
  
  if (fabs(vbat - vsys) < 0.1f && fabs(ibat) < 0.01f) {
    Serial.println(formatField("Analysis:", "VBAT ≈ VSYS with no current flow"));
    Serial.println(formatField("Likely Cause:", "Power path conducting, no real battery connected"));
    
    if (fault0.vbat_ovp) {
      Serial.println(formatField("VBAT OVP Cause:", "VSYS voltage on battery terminal"));
    }
  } else if (ibat > 0.01f) {
    Serial.println(formatField("Analysis:", "Battery charging (" + String(ibat, 3) + "A)"));
  } else if (ibat < -0.01f) {
    Serial.println(formatField("Analysis:", "Battery discharging (" + String(-ibat, 3) + "A)"));
  } else {
    Serial.println(formatField("Analysis:", "Battery idle"));
  }
  
  // Mode status
  printSubsectionHeader("Operating Modes");
  Serial.println(formatField("HIZ Mode:", formatStatusClean(_mppt.api.getHIZMode())));
  Serial.println(formatField("Backup Mode:", formatStatusClean(_mppt.api.getBackupMode())));
  Serial.println(formatField("Ship Mode:", formatStatusClean(_mppt.api.getShipMode())));
}

void ModbeeMpptDebug::printRawRegisters() {
  printSectionHeader("RAW REGISTER VALUES", 80);
  
  // Get raw register values directly from BQ25798 for hex display
  uint8_t status0 = _mppt._bq25798.getChargerStatus0();
  uint8_t status1 = _mppt._bq25798.getChargerStatus1();
  uint8_t status2 = _mppt._bq25798.getChargerStatus2();
  uint8_t status3 = _mppt._bq25798.getChargerStatus3();
  uint8_t status4 = _mppt._bq25798.getChargerStatus4();
  uint8_t fault0 = _mppt._bq25798.getFaultStatus0();
  uint8_t fault1 = _mppt._bq25798.getFaultStatus1();
  
  printSubsectionHeader("Status Registers (Hex)");
  char statusBuffer[64];
  snprintf(statusBuffer, sizeof(statusBuffer), "0x%02X  0x%02X  0x%02X  0x%02X  0x%02X", 
           status0, status1, status2, status3, status4);
  Serial.println(formatField("Status 0-4:", String(statusBuffer)));
  
  printSubsectionHeader("Fault Registers (Hex)");
  char faultBuffer[32];
  snprintf(faultBuffer, sizeof(faultBuffer), "0x%02X  0x%02X", fault0, fault1);
  Serial.println(formatField("Fault 0-1:", String(faultBuffer)));
}

void ModbeeMpptDebug::printRegisterDecoding() {
  printSectionHeader("DETAILED REGISTER DECODING", 80);
  
  // Get all status and fault registers through API
  modbee_status0_t status0 = _mppt.api.getStatus0();
  modbee_status1_t status1 = _mppt.api.getStatus1();
  modbee_status2_t status2 = _mppt.api.getStatus2();
  modbee_status3_t status3 = _mppt.api.getStatus3();
  modbee_status4_t status4 = _mppt.api.getStatus4();
  modbee_fault0_t fault0 = _mppt.api.getFault0();
  modbee_fault1_t fault1 = _mppt.api.getFault1();
  
  // Status 0 breakdown
  printSubsectionHeader("Status 0 Register");
  Serial.println(formatField("IINDPM Status:", status0.iindpm_active ? "TRUE" : "FALSE"));
  Serial.println(formatField("VINDPM Status:", status0.vindpm_active ? "TRUE" : "FALSE"));
  Serial.println(formatField("Watchdog Expired:", status0.watchdog_expired ? "TRUE" : "FALSE"));
  Serial.println(formatField("Power Good:", status0.power_good ? "TRUE" : "FALSE"));
  Serial.println(formatField("AC2 Present:", status0.ac2_present ? "TRUE" : "FALSE"));
  Serial.println(formatField("AC1 Present:", status0.ac1_present ? "TRUE" : "FALSE"));
  Serial.println(formatField("VBUS Present:", status0.vbus_present ? "TRUE" : "FALSE"));
  
  // Status 1 breakdown  
  printSubsectionHeader("Status 1 Register");
  Serial.println(formatField("Charge State:", _mppt.api.getChargeStateString()));
  Serial.println(formatField("VBUS Status:", String(status1.vbus_status)));
  Serial.println(formatField("BC1.2 Done:", status1.bc12_done ? "TRUE" : "FALSE"));
  
  // Status 2 breakdown
  printSubsectionHeader("Status 2 Register");
  Serial.println(formatField("VBAT Present:", status2.battery_present ? "TRUE" : "FALSE"));
  Serial.println(formatField("DPDM Ongoing:", status2.dpdm_detection_ongoing ? "TRUE" : "FALSE"));
  Serial.println(formatField("Thermal Regulation:", status2.thermal_regulation ? "TRUE" : "FALSE"));
  Serial.println(formatField("ICO Status:", String(status2.ico_status)));
  
  // Status 3 breakdown
  printSubsectionHeader("Status 3 Register");
  Serial.println(formatField("ACRB2 Active:", status3.acrb2_active ? "TRUE" : "FALSE"));
  Serial.println(formatField("ACRB1 Active:", status3.acrb1_active ? "TRUE" : "FALSE"));
  Serial.println(formatField("ADC Conversion Done:", status3.adc_conversion_done ? "TRUE" : "FALSE"));
  Serial.println(formatField("VSYS Regulation:", status3.vsys_regulation ? "TRUE" : "FALSE"));
  
  // Status 4 breakdown
  printSubsectionHeader("Status 4 Register");
  Serial.println(formatField("TS Cold:", status4.ts_cold ? "TRUE" : "FALSE"));
  Serial.println(formatField("TS Cool:", status4.ts_cool ? "TRUE" : "FALSE"));
  Serial.println(formatField("TS Warm:", status4.ts_warm ? "TRUE" : "FALSE"));
  Serial.println(formatField("TS Hot:", status4.ts_hot ? "TRUE" : "FALSE"));
  
  // Fault 0 breakdown
  printSubsectionHeader("Fault 0 Register");
  Serial.println(formatField("VAC1 OVP:", fault0.vac1_ovp ? "TRUE" : "FALSE"));
  Serial.println(formatField("VAC2 OVP:", fault0.vac2_ovp ? "TRUE" : "FALSE"));
  Serial.println(formatField("Converter OCP:", fault0.converter_ocp ? "TRUE" : "FALSE"));
  Serial.println(formatField("IBAT OCP:", fault0.ibat_ocp ? "TRUE" : "FALSE"));
  Serial.println(formatField("IBUS OCP:", fault0.ibus_ocp ? "TRUE" : "FALSE"));
  Serial.println(formatField("VBAT OVP:", fault0.vbat_ovp ? "TRUE" : "FALSE"));
  Serial.println(formatField("VBUS OVP:", fault0.vbus_ovp ? "TRUE" : "FALSE"));
  Serial.println(formatField("IBAT Regulation:", fault0.ibat_regulation ? "TRUE" : "FALSE"));
  
  // Fault 1 breakdown
  printSubsectionHeader("Fault 1 Register");
  Serial.println(formatField("Thermal Shutdown:", fault1.thermal_shutdown ? "TRUE" : "FALSE"));
  Serial.println(formatField("OTG UVP:", fault1.otg_uvp ? "TRUE" : "FALSE"));
  Serial.println(formatField("OTG OVP:", fault1.otg_ovp ? "TRUE" : "FALSE"));
  Serial.println(formatField("VSYS OVP:", fault1.vsys_ovp ? "TRUE" : "FALSE"));
  Serial.println(formatField("VSYS Short:", fault1.vsys_short ? "TRUE" : "FALSE"));
}

// ========================================================================
// UTILITY FUNCTIONS
// ========================================================================

void ModbeeMpptDebug::printSectionHeader(const String& title, int width) {
  String border = "";
  for(int i = 0; i < width; i++) {
    border += "=";
  }
  Serial.println();
  Serial.println(border);
  Serial.println(title);
  Serial.println(border);
}

void ModbeeMpptDebug::printSubsectionHeader(const String& title) {
  Serial.println();
  Serial.println("  ------ " + title + " ------");
}

void ModbeeMpptDebug::printMainHeader(const String& title, int width) {
  String border = "";
  for(int i = 0; i < width; i++) {
    border += "=";
  }
  Serial.println();
  Serial.println(border);
  Serial.println("  " + title);
  Serial.println(border);
}

void ModbeeMpptDebug::printSectionDivider() {
  Serial.println();
  String divider = "";
  for(int i = 0; i < 80; i++) {
    divider += "#";
  }
  Serial.println(divider);
}

String ModbeeMpptDebug::formatStatus(const String& label, bool status) {
  return label + ": " + (status ? "ENABLED" : "DISABLED");
}

String ModbeeMpptDebug::formatStatusClean(bool status) {
  return status ? "ENABLED" : "DISABLED";
}

String ModbeeMpptDebug::formatField(const String& fieldName, const String& value) {
  // Simple table approach: fixed positions like a proper table
  String result = "  ";
  result += fieldName;
  
  // Always pad to at least position 35 from start of line
  while(result.length() < 35) {
    result += " ";
  }
  
  result += value;
  return result;
}

void ModbeeMpptDebug::printComprehensiveBatteryStatus() {
  printSectionHeader("COMPREHENSIVE BATTERY STATUS", 80);
  
  // Get comprehensive battery status using the single API instance
  modbee_battery_status_t status = _mppt.api.getComprehensiveBatteryStatus();
  
  // Voltage readings
  printSubsectionHeader("Battery Voltages");
  Serial.println(formatField("Charging Voltage:", String(status.charging_voltage, 3) + "V (while charging)"));
  Serial.println(formatField("True Battery Voltage:", String(status.true_voltage, 3) + "V (charging stopped)"));
  Serial.println(formatField("Voltage Difference:", String(status.charging_voltage - status.true_voltage, 3) + "V"));
  
  // State of Charge
  printSubsectionHeader("State of Charge (SOC)");
  Serial.println(formatField("Actual SOC:", String(status.actual_soc, 1) + "% (full battery range)"));
  Serial.println(formatField("Usable SOC:", String(status.usable_soc, 1) + "% (system operating range)"));
  Serial.println(formatField("SOC Difference:", String(status.actual_soc - status.usable_soc, 1) + "%"));
  
  // Current and Power
  printSubsectionHeader("Battery Current & Power");
  Serial.println(formatField("Current:", String(status.current, 3) + "A (" + status.state + ")"));
  float power = status.true_voltage * status.current;
  Serial.println(formatField("Power:", String(power, 2) + "W"));
  
  // Temperature and State
  printSubsectionHeader("Battery Condition");
  Serial.println(formatField("Temperature:", String(status.temperature, 1) + "°C"));
  Serial.println(formatField("State:", status.state));
  
  // Voltage Range Information
  printSubsectionHeader("Configured Voltage Ranges");
  // Note: These would need to be exposed as public getters if needed
  // For now, we'll skip showing the internal voltage ranges
  Serial.println(formatField("Info:", "See system configuration for voltage ranges"));
}
