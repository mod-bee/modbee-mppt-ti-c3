/*!
 * @file ModbeeMpptAPI.h
 * 
 * @brief High-level MPPT API for safe and easy control of the BQ25798 charger
 * 
 * This API provides a user-friendly interface to the BQ25798 charger with built-in
 * safety limits, proper value clamping, and meaningful status/fault reporting.
 * Works with an existing ModbeeMPPT instance.
 */

#ifndef MODBEE_MPPT_API_H
#define MODBEE_MPPT_API_H

#include "ModbeeMpptGlobal.h"

// Forward declaration
class ModbeeMPPT;

// BQ25798 Safe Operating Limits (from datasheet)
#define MODBEE_MIN_CHARGE_VOLTAGE     3.0f     // Minimum charge voltage (V)
#define MODBEE_MAX_CHARGE_VOLTAGE     18.8f    // Maximum charge voltage (V)
#define MODBEE_MIN_CHARGE_CURRENT     0.0f     // Minimum charge current (A)
#define MODBEE_MAX_CHARGE_CURRENT     5.0f     // Maximum charge current (A)
#define MODBEE_MIN_INPUT_VOLTAGE      3.6f     // Minimum input voltage (V)
#define MODBEE_MAX_INPUT_VOLTAGE      22.0f    // Maximum input voltage (V)
#define MODBEE_MIN_INPUT_CURRENT      0.1f     // Minimum input current (A) - datasheet min 100mA
#define MODBEE_MAX_INPUT_CURRENT      3.3f     // Maximum input current (A) - CORRECTED: datasheet max 3300mA
#define MODBEE_MIN_SYSTEM_VOLTAGE     2.5f     // Minimum system voltage (V) - datasheet min 2500mV
#define MODBEE_MAX_SYSTEM_VOLTAGE     16.0f    // Maximum system voltage (V) - datasheet max 16000mV
#define MODBEE_MIN_BATTERY_VOLTAGE    2.5f     // Minimum battery voltage (V)
#define MODBEE_MAX_BATTERY_VOLTAGE    18.8f    // Maximum battery voltage (V)
#define MODBEE_VSYS_VBAT_VDROP        1.0f     // VSYS - VBAT voltage drop threshold (V)

// Battery Type Voltage Ranges
#define MODBEE_LIFEPO4_MIN_VOLTAGE    2.5f     // LiFePO4 minimum cell voltage
#define MODBEE_LIFEPO4_MAX_VOLTAGE    3.65f    // LiFePO4 maximum cell voltage
#define MODBEE_LIPO_MIN_VOLTAGE       2.8f     // Li-Po minimum cell voltage
#define MODBEE_LIPO_MAX_VOLTAGE       4.2f     // Li-Po maximum cell voltage
#define MODBEE_LEAD_ACID_MIN_VOLTAGE  1.8f     // Lead acid minimum cell voltage
#define MODBEE_LEAD_ACID_MAX_VOLTAGE  2.4f     // Lead acid maximum cell voltage

// Battery chemistry types
typedef enum {
  MODBEE_BATTERY_LIFEPO4 = 0,
  MODBEE_BATTERY_LIPO,
  MODBEE_BATTERY_LEAD_ACID,
  MODBEE_BATTERY_CUSTOM
} modbee_battery_type_t;

// ADC configuration types  
typedef enum {
  MODBEE_ADC_CONTINUOUS = 0,
  MODBEE_ADC_ONE_SHOT = 1
} modbee_adc_mode_t;

typedef enum {
  MODBEE_ADC_AVG_1 = 0,
  MODBEE_ADC_AVG_4 = 1,
  MODBEE_ADC_AVG_16 = 2,
  MODBEE_ADC_AVG_64 = 3
} modbee_adc_avg_t;

typedef enum {
  MODBEE_ADC_RES_15BIT = 0,
  MODBEE_ADC_RES_14BIT = 1,
  MODBEE_ADC_RES_13BIT = 2,
  MODBEE_ADC_RES_12BIT = 3
} modbee_adc_res_t;

// Timer configuration types
typedef enum {
  MODBEE_TIMER_5HR = 0,
  MODBEE_TIMER_8HR = 1,
  MODBEE_TIMER_12HR = 2,
  MODBEE_TIMER_24HR = 3    // CORRECTED: Was 20HR, should be 24HR per datasheet
} modbee_charge_timer_t;

typedef enum {
  MODBEE_PRECHARGE_TIMER_2HR = 0,     // CORRECTED: 2 hours (default)
  MODBEE_PRECHARGE_TIMER_0_5HR = 1    // CORRECTED: 0.5 hours
} modbee_precharge_timer_t;

typedef enum {
  MODBEE_TOPOFF_TIMER_DISABLED = 0,
  MODBEE_TOPOFF_TIMER_15MIN = 1,
  MODBEE_TOPOFF_TIMER_30MIN = 2,
  MODBEE_TOPOFF_TIMER_45MIN = 3
} modbee_topoff_timer_t;

typedef enum {
  MODBEE_VBAT_LOWV_15_PERCENT = 0,   ///< 15% of VREG
  MODBEE_VBAT_LOWV_62_2_PERCENT = 1, ///< 62.2% of VREG  
  MODBEE_VBAT_LOWV_66_7_PERCENT = 2, ///< 66.7% of VREG
  MODBEE_VBAT_LOWV_71_4_PERCENT = 3  ///< 71.4% of VREG (default)
} modbee_vbat_lowv_t;

// MPPT VOC configuration types (matching BQ25798 datasheet)
typedef enum {
  MODBEE_VOC_PERCENT_56_25 = 0,    // 56.25% of VOC
  MODBEE_VOC_PERCENT_62_5 = 1,     // 62.5% of VOC
  MODBEE_VOC_PERCENT_68_75 = 2,    // 68.75% of VOC
  MODBEE_VOC_PERCENT_75 = 3,       // 75% of VOC
  MODBEE_VOC_PERCENT_81_25 = 4,    // 81.25% of VOC
  MODBEE_VOC_PERCENT_87_5 = 5,     // 87.5% of VOC (default)
  MODBEE_VOC_PERCENT_93_75 = 6,    // 93.75% of VOC
  MODBEE_VOC_PERCENT_100 = 7       // 100% of VOC
} modbee_voc_percent_t;

typedef enum {
  MODBEE_VOC_DELAY_50MS = 0,       // 50ms delay
  MODBEE_VOC_DELAY_300MS = 1,      // 300ms delay (default)
  MODBEE_VOC_DELAY_2S = 2,         // 2 second delay
  MODBEE_VOC_DELAY_5S = 3          // 5 second delay
} modbee_voc_delay_t;

typedef enum {
  MODBEE_VOC_RATE_30S = 0,         // 30 second measurement rate
  MODBEE_VOC_RATE_2MIN = 1,        // 2 minute measurement rate (default)
  MODBEE_VOC_RATE_10MIN = 2,       // 10 minute measurement rate
  MODBEE_VOC_RATE_30MIN = 3        // 30 minute measurement rate
} modbee_voc_rate_t;

// Power measurement structure
typedef struct {
  float voltage;      // Voltage (V)
  float current;      // Current (A)
  float power;        // Power (W)
  bool valid;         // Measurement validity
} modbee_power_data_t;

// Comprehensive battery status structure
typedef struct {
  float charging_voltage;     // Battery voltage while charging (V)
  float true_voltage;         // True battery voltage (charging stopped) (V)
  float actual_soc;           // Actual SOC based on full battery range (%)
  float usable_soc;           // Usable SOC based on system operating range (%)
  float current;              // Battery current (A, positive = charging)
  float temperature;          // Battery temperature (°C)
  String state;               // Battery state: "Charging", "Discharging", "Idle"
} modbee_battery_status_t;

// Charge state enumeration
typedef enum {
  MODBEE_CHARGE_NOT_CHARGING = 0,
  MODBEE_CHARGE_TRICKLE = 1,
  MODBEE_CHARGE_PRECHARGE = 2,
  MODBEE_CHARGE_FAST_CC = 3,
  MODBEE_CHARGE_TAPER_CV = 4,
  MODBEE_CHARGE_RESERVED = 5,
  MODBEE_CHARGE_TOPOFF = 6,
  MODBEE_CHARGE_DONE = 7
} modbee_charge_state_t;

// VBUS status enumeration (Status 1 bits 4:1)
typedef enum {
  MODBEE_VBUS_NO_INPUT = 0x0,
  MODBEE_VBUS_USB_SDP = 0x1,
  MODBEE_VBUS_USB_CDP = 0x2,
  MODBEE_VBUS_USB_DCP = 0x3,
  MODBEE_VBUS_HVDCP = 0x4,
  MODBEE_VBUS_UNKNOWN = 0x5,
  MODBEE_VBUS_NON_STANDARD = 0x6,
  MODBEE_VBUS_OTG = 0x7,
  MODBEE_VBUS_NOT_QUALIFIED = 0x8,
  MODBEE_VBUS_DIRECT_POWER = 0xB,
  MODBEE_VBUS_BACKUP = 0xC
} modbee_vbus_status_t;

// ICO status enumeration (Status 2 bits 7:6)
typedef enum {
  MODBEE_ICO_DISABLED = 0,
  MODBEE_ICO_IN_PROGRESS = 1,
  MODBEE_ICO_MAX_CURRENT = 2,
  MODBEE_ICO_RESERVED = 3
} modbee_ico_status_t;

// Watchdog timer enumeration (from BQ25798 datasheet)
typedef enum {
  MODBEE_WATCHDOG_DISABLE = 0,
  MODBEE_WATCHDOG_40S = 1,
  MODBEE_WATCHDOG_80S = 2,
  MODBEE_WATCHDOG_160S = 3
} modbee_watchdog_timer_t;

// PWM switching frequency enumeration (from BQ25798 datasheet)
typedef enum {
  MODBEE_PWM_FREQ_1_5MHZ = 0,  // 1.5 MHz (default, quieter operation)
  MODBEE_PWM_FREQ_750KHZ = 1   // 750 kHz (audible range, may cause noise)
} modbee_pwm_freq_t;

// PFM (Pulse Frequency Modulation) mode settings
// PFM reduces switching frequency at light loads for efficiency but can cause audible noise
typedef enum {
  MODBEE_PFM_DISABLE = 0,  // Disable PFM, stay in PWM mode (recommended for quiet operation)
  MODBEE_PFM_ENABLE = 1    // Enable PFM for better light-load efficiency (may cause noise)
} modbee_pfm_mode_t;

// OOA (Out of Audio) feature settings
// When enabled, limits minimum switching frequency to 25kHz to avoid audible noise
typedef enum {
  MODBEE_OOA_DISABLE = 0,  // Disable OOA, switching frequency can drop below 25kHz
  MODBEE_OOA_ENABLE = 1    // Enable OOA, maintain minimum 25kHz switching frequency
} modbee_ooa_mode_t;

// Status Register 0 (REG1B) - Charger Status 0
typedef struct {
  bool vbus_present;           // Bit 0: VBUS present status
  bool ac1_present;            // Bit 1: VAC1 present status
  bool ac2_present;            // Bit 2: VAC2 present status
  bool power_good;             // Bit 3: Power good status
  // Bit 4: Reserved
  bool watchdog_expired;       // Bit 5: I2C watchdog timer expired
  bool vindpm_active;          // Bit 6: VINDPM regulation active
  bool iindpm_active;          // Bit 7: IINDPM regulation active
} modbee_status0_t;

// Status Register 1 (REG1C) - Charger Status 1
typedef struct {
  bool bc12_done;              // Bit 0: BC1.2 detection complete
  modbee_vbus_status_t vbus_status; // Bits 4:1: VBUS status
  modbee_charge_state_t charge_state; // Bits 7:5: Charge status
} modbee_status1_t;

// Status Register 2 (REG1D) - Charger Status 2
typedef struct {
  bool battery_present;        // Bit 0: Battery present (VBAT > VBAT_UVLOZ)
  bool dpdm_detection_ongoing; // Bit 1: D+/D- detection ongoing
  bool thermal_regulation;     // Bit 2: IC in thermal regulation
  // Bits 5:3: Reserved
  modbee_ico_status_t ico_status; // Bits 7:6: ICO status
} modbee_status2_t;

// Status Register 3 (REG1E) - Charger Status 3
typedef struct {
  // Bit 0: Reserved
  bool precharge_timer_expired;  // Bit 1: Pre-charge timer expired
  bool trickle_timer_expired;    // Bit 2: Trickle charge timer expired
  bool charge_timer_expired;     // Bit 3: Fast charge timer expired
  bool vsys_regulation;          // Bit 4: VSYSMIN regulation active
  bool adc_conversion_done;      // Bit 5: ADC conversion complete
  bool acrb1_active;             // Bit 6: ACFET1-RBFET1 placed
  bool acrb2_active;             // Bit 7: ACFET2-RBFET2 placed
} modbee_status3_t;

// Status Register 4 (REG1F) - Charger Status 4
typedef struct {
  bool ts_hot;                 // Bit 0: TS in hot range (>T5)
  bool ts_warm;                // Bit 1: TS in warm range (T3-T5)
  bool ts_cool;                // Bit 2: TS in cool range (T1-T2)
  bool ts_cold;                // Bit 3: TS in cold range (<T1)
  bool vbat_otg_low;           // Bit 4: Battery voltage too low for OTG
  // Bits 7:5: Reserved
} modbee_status4_t;

// Fault Register 0 (REG20) - FAULT Status 0
typedef struct {
  bool vac1_ovp;               // Bit 0: VAC1 overvoltage protection
  bool vac2_ovp;               // Bit 1: VAC2 overvoltage protection
  bool converter_ocp;          // Bit 2: Converter overcurrent protection
  bool ibat_ocp;               // Bit 3: IBAT overcurrent protection
  bool ibus_ocp;               // Bit 4: IBUS overcurrent protection
  bool vbat_ovp;               // Bit 5: VBAT overvoltage protection
  bool vbus_ovp;               // Bit 6: VBUS overvoltage protection
  bool ibat_regulation;        // Bit 7: IBAT regulation active
} modbee_fault0_t;

// Fault Register 1 (REG21) - FAULT Status 1
typedef struct {
  // Bits 1:0: Reserved
  bool thermal_shutdown;       // Bit 2: IC thermal shutdown
  // Bit 3: Reserved
  bool otg_uvp;                // Bit 4: OTG undervoltage protection
  bool otg_ovp;                // Bit 5: OTG overvoltage protection
  bool vsys_ovp;               // Bit 6: VSYS overvoltage protection
  bool vsys_short;             // Bit 7: VSYS short circuit protection
} modbee_fault1_t;

// Combined Status Structure
typedef struct {
  modbee_status0_t status0;
  modbee_status1_t status1;
  modbee_status2_t status2;
  modbee_status3_t status3;
  modbee_status4_t status4;
  modbee_fault0_t fault0;
  modbee_fault1_t fault1;
} modbee_complete_status_t;

class ModbeeMpptAPI {
public:
  /**
   * @brief Enable or disable Input Current Optimizer (ICO)
   * @param enable True to enable ICO, false to disable
   * @return True if successful
   */
  bool setICOEnable(bool enable);
  // Stats update function
  void updateStats();

  // Constructor - takes reference to existing ModbeeMPPT instance
  ModbeeMpptAPI(ModbeeMPPT& mppt);
  
  // ========================================================================
  // POWER MEASUREMENT FUNCTIONS
  // ========================================================================
  
  /*!
   * @brief Get VBUS (input) power measurements
   * @return Power data structure with voltage, current, and calculated power
   */
  modbee_power_data_t getVbusPower();
  
  /*!
   * @brief Get battery power measurements
   * @return Power data structure with voltage, current, and calculated power
   */
  modbee_power_data_t getBatteryPower();
  
  /*!
   * @brief Get system power measurements with accurate cross-domain calculation
   * 
   * Properly accounts for voltage domain differences in buck-boost converter.
   * System power = Input power - Battery power (preserves energy conservation).
   * 
   * @return Power data structure with corrected voltage, current, and power
   */
  modbee_power_data_t getSystemPower();
  
  /*!
   * @brief Get VAC1 input power (real input voltage measurement)
   * @return Power data structure
   */
  modbee_power_data_t getVAC1Power();
  
  /*!
   * @brief Get VAC2 input power (second input voltage measurement)
   * @return Power data structure
   */
  modbee_power_data_t getVAC2Power();
  
  /*!
   * @brief Get overall system efficiency (output power / input power)
   * @return Efficiency as percentage (0.0 - 100.0)
   */
  float getEfficiency();
  
  // ========================================================================
  // INDIVIDUAL VOLTAGE AND CURRENT MEASUREMENTS
  // ========================================================================
  
  /*!
   * @brief Get VBUS voltage (input voltage)
   * @return Voltage in volts
   */
  float getVbusVoltage();
  
  /*!
   * @brief Get IBUS current (input current)
   * @return Current in amps
   */
  float getIbusCurrent();
  
  /*!
   * @brief Get battery voltage
   * @return Voltage in volts
   */
  float getBatteryVoltage();
  
  /*!
   * @brief Get battery current (positive = charging)
   * @return Current in amps
   */
  float getBatteryCurrent();
  
  /*!
   * @brief Get system voltage
   * @return Voltage in volts
   */
  float getSystemVoltage();
  
  /*!
   * @brief Get system current (calculated from IBUS - IBAT)
   * @return Current in amps
   */
  float getSystemCurrent();
  
  /*!
   * @brief Get VAC1 voltage (real input voltage)
   * @return Voltage in volts
   */
  float getVAC1Voltage();
  
  /*!
   * @brief Get VAC2 voltage (real input voltage)
   * @return Voltage in volts
   */
  float getVAC2Voltage();
  
  // ========================================================================
  // BATTERY FUNCTIONS
  // ========================================================================
  
  /*!
   * @brief Get battery state of charge percentage
   * @return SoC percentage (0.0 - 100.0), based on configured voltage range
   */
  float getBatteryChargePercent();
  
  /*!
   * @brief Get battery temperature from BQ25798 die temperature
   * @return Temperature in Celsius
   */
  float getBatteryTemperature();
  
  /*!
   * @brief Get raw TS ADC reading as percentage (for debugging)
   * @return TS ADC reading as percentage (0-100%)
   */
  float getRawTSPercent();
   /*!
   * @brief Update true battery voltage measurement (start measurement process)
   * 
   * This function initiates the true battery voltage measurement process if one
   * is not already in progress. The measurement temporarily disables charging 
   * and forces a small discharge current to measure the true no-load battery voltage.
   * 
   * This is typically called from the main loop every 10 seconds to keep
   * the true battery voltage reading fresh.
   */
  void updateTrueBatteryVoltage();

   /*!
   * @brief Get true battery voltage (non-blocking)
   * 
   * This function returns the most recent true battery voltage reading.
   * The measurement is handled automatically by the ModbeeMPPT class.
   * 
   * @return True battery voltage in volts
   */
  float getTrueBatteryVoltage();
  
  /*!
   * @brief Update method called from ModbeeMPPT loop
   * 
   * This handles all non-blocking state machines and timers.
   * Must be called regularly from the main loop.
   */
  void update();

  /*!
   * @brief Get battery voltage while charging (charging terminal voltage)
   * 
   * This is the voltage measured at the battery terminals while charging is active.
   * This voltage will be higher than the true battery voltage due to charging current.
   * 
   * @return Battery charging voltage in volts
   */
  float getBatteryChargingVoltage();
  
  /*!
   * @brief Get actual battery state of charge based on full battery voltage range
   * 
   * This SOC is calculated using the true battery voltage and the full voltage range
   * of the configured battery chemistry (e.g., 2.5V to 4.2V for Li-Ion per cell).
   * This represents the absolute battery charge state.
   * 
   * @return Actual battery SOC percentage (0.0 - 100.0)
   */
  float getActualBatterySOC();
  
  /*!
   * @brief Get usable battery state of charge based on system operating range
   * 
   * This SOC is calculated using the true battery voltage but limited to the
   * system's usable voltage range (min system voltage to charge voltage).
   * This represents the practically usable energy in the system.
   * 
   * @return Usable battery SOC percentage (0.0 - 100.0)
   */
  float getUsableBatterySOC();
  
  /*!
   * @brief Get comprehensive battery status with all voltage and SOC readings
   * 
   * This function provides a complete battery status including charging voltage,
   * true battery voltage, actual SOC, usable SOC, current, temperature, and state.
   * 
   * @return Structure containing all battery voltage and SOC readings
   */
  modbee_battery_status_t getComprehensiveBatteryStatus();
  
  /*!
   * @brief Set battery type for automatic voltage limits
   * @param type Battery chemistry type
   * @param cell_count Number of cells in series
   * @return True if successful
   */
  bool setBatteryType(modbee_battery_type_t type, uint8_t cell_count);
  
  /*!
   * @brief Set custom battery voltage range for SoC calculation
   * @param min_voltage Minimum battery voltage (V)
   * @param max_voltage Maximum battery voltage (V)
   * @return True if successful
   */
  bool setBatteryVoltageRange(float min_voltage, float max_voltage);
  
  // ========================================================================
  // CHARGING CONTROL FUNCTIONS (with safety clamping)
  // ========================================================================
  
  /*!
   * @brief Set charge voltage limit (with safety clamping)
   * @param voltage Charge voltage limit in volts
   * @return True if successful
   */
  bool setChargeVoltage(float voltage);
  
  /*!
   * @brief Set charge current limit (with safety clamping)
   * @param current Charge current limit in amps
   * @return True if successful
   */
  bool setChargeCurrent(float current);
  
  /*!
   * @brief Get termination current (ITERM)
   * @return Termination current in amps
   */
  float getTerminationCurrent();
  
  /*!
   * @brief Set termination current (ITERM) 
   * @param current Termination current in amps (0.04A - 1.0A)
   * @return True if successful
   */
  bool setTerminationCurrent(float current);
  
  /*!
   * @brief Get recharge threshold voltage offset (VRECHG)
   * @return Recharge threshold offset in volts (below charge voltage)
   */
  float getRechargeThreshold();
  
  /*!
   * @brief Set recharge threshold voltage offset (VRECHG)
   * @param offset Recharge threshold offset in volts (0.05V - 0.8V below charge voltage)
   * @return True if successful
   */
  bool setRechargeThreshold(float offset);
  
  /*!
   * @brief Get precharge current limit
   * @return Precharge current in amps
   */
  float getPrechargeCurrent();
  
  /*!
   * @brief Set precharge current limit (IPRECHG)
   * @param current Precharge current in amps (0.04A - 2.0A)
   * @return True if successful
   */
  bool setPrechargeCurrent(float current);
  
  /*!
   * @brief Get battery low voltage threshold for precharge-to-fast-charge transition
   * @return Current VBAT_LOWV setting
   */
  modbee_vbat_lowv_t getPrechargeVoltageThreshold();
  
  /*!
   * @brief Set battery low voltage threshold for precharge-to-fast-charge transition (VBAT_LOWV)
   * @param threshold Voltage threshold as percentage of VREG
   * @return True if successful
   */
  bool setPrechargeVoltageThreshold(modbee_vbat_lowv_t threshold);
  
  /*!
   * @brief Set input current limit (with safety clamping)
   * @param current Input current limit in amps
   * @return True if successful
   */
  bool setInputCurrentLimit(float current);
  
  /*!
   * @brief Set input voltage limit (with safety clamping)
   * @param voltage Input voltage limit in volts (3.5V - 26V)
   * @return True if successful
   */
  bool setInputVoltageLimit(float voltage);
  
  /*!
   * @brief Get input voltage limit setting
   * @return Input voltage limit in volts
   */
  float getInputVoltageLimit();
  
  /*!
   * @brief Set minimum system voltage (with safety clamping)
   * @param voltage Minimum system voltage in volts
   * @return True if successful  
   */
  bool setMinSystemVoltage(float voltage);
  
  /*!
   * @brief Get minimum system voltage setting
   * @return Minimum system voltage in volts
   */
  float getMinSystemVoltage();
  
  /*!
   * @brief Set VAC overvoltage protection threshold
   * @param voltage OVP threshold in volts (7V, 12V, 22V, or 26V)
   * @return True if successful
   */
  bool setVACOVP(float voltage);
  
  /*!
   * @brief Get VAC overvoltage protection threshold
   * @return OVP threshold in volts
   */
  float getVACOVP();
  
  /*!
   * @brief Get the configured cell count
   * @return Number of cells (1-4)
   */
  uint8_t getCellCount();
  
  /*!
   * @brief Enable or disable charging
   * @param enable True to enable charging
   * @return True if successful
   */
  bool setChargeEnable(bool enable);
  
  /*!
   * @brief Get charge enable status
   * @return True if charging is enabled
   */
  bool getChargeEnable();
  
  /*!
   * @brief Get charge voltage setting
   * @return Charge voltage in volts
   */
  float getChargeVoltage();
  
  /*!
   * @brief Get charge current setting
   * @return Charge current in amperes
   */
  float getChargeCurrent();
  
  /*!
   * @brief Get input current limit setting
   * @return Input current limit in amperes
   */
  float getInputCurrentLimit();
  
  /*!
   * @brief Get watchdog timer enable status
   * @return True if watchdog is enabled
   */
  bool getWatchdogEnable();
  
  /*!
   * @brief Get watchdog timer period setting
   * @return Watchdog timer period
   */
  modbee_watchdog_timer_t getWatchdogTimer();
  
  /*!
   * @brief Set HIZ (High Impedance) mode
   * HIZ mode disconnects input from system - when enabled, VSYS is not powered!
   * @param enable True to enable HIZ mode (disables system power), false to enable system power
   * @return True if successful
   */
  bool setHIZMode(bool enable);
  
  /*!
   * @brief Get HIZ mode status
   * @return True if HIZ mode is enabled (system power disabled)
   */
  bool getHIZMode();
  
  /*!
   * @brief Set backup mode enable (automatic power path switching)
   * @param enable True to enable backup mode
   * @return True if successful
   */
  bool setBackupMode(bool enable);
  
  /*!
   * @brief Get backup mode status
   * @return True if backup mode is enabled
   */
  bool getBackupMode();
  
  /*!
   * @brief Get ship mode status
   * @return True if chip is in ship mode
   */
  bool getShipMode();
  
  // ========================================================================
  // STATUS AND FAULT FUNCTIONS
  // ========================================================================
  
  /*!
   * @brief Get human-readable charging state string
   * @return Charging state as string
   */
  String getChargeStateString();
  
  /*!
   * @brief Check if battery is currently charging
   * 
   * Returns true if the BQ25798 is in any charging mode (trickle, precharge, 
   * fast charge, taper, or topoff). Returns false if not charging or done.
   * 
   * @return True if actively charging
   */
  bool isCharging();
  
  /*!
   * @brief Check if any faults are present
   * @return True if faults are present
   */
  bool hasFaults();
  
  /*!
   * @brief Get human-readable fault status string
   * @return Fault description string
   */
  String getFaultString();
  
  /*!
   * @brief Get all fault status registers
   * @param fault0 Pointer to store Fault Status 0
   * @param fault1 Pointer to store Fault Status 1
   */
  void getAllFaultStatus(uint8_t* fault0, uint8_t* fault1);
  
  /*!
   * @brief Check for specific input overvoltage faults
   * @return True if VAC1, VAC2, or VBUS overvoltage detected
   */
  bool hasInputOvervoltageFault();
  
  /*!
   * @brief Check for overcurrent faults
   * @return True if any overcurrent fault detected
   */
  bool hasOvercurrentFault();
  
  /*!
   * @brief Check for thermal faults
   * @return True if thermal shutdown or regulation active
   */
  bool hasThermalFault();
  
  // ========================================================================
  // ADC CONTROL FUNCTIONS
  // ========================================================================
  
  /*!
   * @brief Enable or disable ADC
   * @param enable True to enable ADC  
   * @return True if successful
   */
  bool setADCEnable(bool enable);
  
  /*!
   * @brief Get ADC enable status
   * @return True if ADC is enabled
   */
  bool getADCEnable();
  
  /*!
   * @brief Set ADC conversion mode
   * @param mode Continuous or one-shot conversion
   * @return True if successful
   */
  bool setADCMode(modbee_adc_mode_t mode);
  
  /*!
   * @brief Set ADC averaging samples
   * @param averaging Number of samples to average
   * @return True if successful
   */
  bool setADCAveraging(modbee_adc_avg_t averaging);
  
  /*!
   * @brief Set ADC resolution
   * @param resolution ADC resolution setting
   * @return True if successful
   */
  bool setADCResolution(modbee_adc_res_t resolution);
  
  /*!
   * @brief Configure ADC with all parameters at once
   * @param resolution ADC resolution (12-15 bits)
   * @param averaging ADC averaging (1-64 samples) 
   * @param mode ADC mode (continuous or one-shot)
   * @return True if configuration successful
   */
  bool configureADC(modbee_adc_res_t resolution = MODBEE_ADC_RES_15BIT,
                    modbee_adc_avg_t averaging = MODBEE_ADC_AVG_1,
                    modbee_adc_mode_t mode = MODBEE_ADC_CONTINUOUS);

  /*!
   * @brief Debug function to read raw ADC control register
   * @return Raw ADC control register value (for debugging)
   */
  uint8_t getADCControlRegister();
  
  /*!
   * @brief Check if ADC conversion is complete (one-shot mode only)
   * @return True if conversion is done
   */
  bool isADCConversionDone();
  
  // ========================================================================
  // TIMER CONTROL FUNCTIONS
  // ========================================================================
  
  /*!
   * @brief Set fast charge timer duration
   * @param timer Timer duration
   * @return True if successful
   */
  bool setFastChargeTimer(modbee_charge_timer_t timer);
  
  /*!
   * @brief Enable or disable fast charge timer
   * @param enable True to enable timer
   * @return True if successful
   */
  bool setFastChargeTimerEnable(bool enable);
  
  /*!
   * @brief Get fast charge timer enable status
   * @return True if timer is enabled
   */
  bool getFastChargeTimerEnable();
  
  /*!
   * @brief Set precharge timer duration
   * @param timer Timer duration
   * @return True if successful
   */
  bool setPrechargeTimer(modbee_precharge_timer_t timer);
  
  /*!
   * @brief Enable or disable precharge timer
   * @param enable True to enable timer
   * @return True if successful
   */
  bool setPrechargeTimerEnable(bool enable);
  
  /*!
   * @brief Get precharge timer enable status
   * @return True if timer is enabled
   */
  bool getPrechargeTimerEnable();
  
  /*!
   * @brief Get top-off timer duration
   * @return Current top-off timer setting
   */
  modbee_topoff_timer_t getTopOffTimer();
  
  /*!
   * @brief Set top-off timer duration
   * @param timer Timer duration
   * @return True if successful
   */
  bool setTopOffTimer(modbee_topoff_timer_t timer);
  
  /*!
   * @brief Enable or disable trickle charge timer
   * @param enable True to enable timer
   * @return True if successful
   */
  bool setTrickleChargeTimerEnable(bool enable);
  
  /*!
   * @brief Get trickle charge timer enable status
   * @return True if timer is enabled
   */
  bool getTrickleChargeTimerEnable();
  
  /*!
   * @brief Enable or disable timer half-rate mode
   * @param enable True to enable half-rate
   * @return True if successful
   */
  bool setTimerHalfRateEnable(bool enable);
  
  /*!
   * @brief Get timer half-rate enable status
   * @return True if half-rate is enabled
   */
  bool getTimerHalfRateEnable();
  
  // ========================================================================
  // MPPT CONTROL FUNCTIONS  
  // ========================================================================
  
  /*!
   * @brief Enable or disable MPPT functionality
   * @param enable True to enable MPPT
   * @return True if successful
   */
  bool setMPPTEnable(bool enable);
  
  /*!
   * @brief Get MPPT enable status
   * @return True if MPPT is enabled
   */
  bool getMPPTEnable();
  
  // ========================================================================
  // SYSTEM CONTROL FUNCTIONS
  // ========================================================================
  
  /*!
   * @brief Enable or disable watchdog timer
   * @param enable True to enable watchdog
   * @return True if successful
   */
  bool setWatchdogEnable(bool enable);
  
  /*!
   * @brief Set watchdog timer period
   * @param timer Watchdog timer period setting
   * @return True if successful
   */
  bool setWatchdogTimer(modbee_watchdog_timer_t timer);

  /*!
   * @brief Reset watchdog timer
   * @return True if successful
   */
  bool resetWatchdog();
  
  /*!
   * @brief Set PWM switching frequency
   * @param frequency PWM frequency setting (1.5MHz recommended for quiet operation)
   * @return True if successful
   */
  bool setPWMFrequency(modbee_pwm_freq_t frequency);
  
  /*!
   * @brief Get PWM switching frequency
   * @return Current PWM frequency setting
   */
  modbee_pwm_freq_t getPWMFrequency();
  
  /*!
   * @brief Set forward PFM (Pulse Frequency Modulation) mode
   * @param enable True to enable PFM (better efficiency), false to disable (quieter operation)
   * @return True if successful
   */
  bool setForwardPFM(bool enable);
  
  /*!
   * @brief Get forward PFM mode status
   * @return True if PFM is enabled
   */
  bool getForwardPFM();
  
  /*!
   * @brief Set forward OOA (Out of Audio) mode
   * @param enable True to enable OOA (maintain 25kHz minimum frequency)
   * @return True if successful
   */
  bool setForwardOOA(bool enable);
  
  /*!
   * @brief Get forward OOA mode status
   * @return True if OOA is enabled
   */
  bool getForwardOOA();
  
  /*!
   * @brief Enable or disable shipping mode
   * @param enable True to enable shipping mode
   * @return True if successful
   */
  bool setShippingMode(bool enable);
  
  /*!
   * @brief Get die temperature
   * @return Temperature in Celsius
   */
  float getDieTemperature();
  
  /*!
   * @brief Detect if a real battery is connected
   * @return True if real battery detected, false if only capacitance or no battery
   */
  bool detectBatteryConnected();
  
  /*!
   * @brief Enable battery discharge current sensing (EN_BAT bit)
   * Required to get negative current readings during discharge
   * @param enable True to enable discharge sensing
   * @return True if successful
   */
  bool setBatteryDischargeSenseEnable(bool enable);
  
  /*!
   * @brief Get battery discharge current sensing status
   * @return True if discharge sensing is enabled
   */
  bool getBatteryDischargeSenseEnable();
  
  // ========================================================================
  // MPPT VOC CONFIGURATION FUNCTIONS
  // ========================================================================
  
  /*!
   * @brief Set MPPT VOC percentage for tracking point
   * @param percent VOC percentage for MPPT operation (56.25% - 100%)
   * @return True if successful
   */
  bool setMPPTVOCPercent(modbee_voc_percent_t percent);
  
  /*!
   * @brief Get current MPPT VOC percentage setting
   * @return Current VOC percentage setting
   */
  modbee_voc_percent_t getMPPTVOCPercent();
  
  /*!
   * @brief Set VOC measurement delay
   * @param delay Delay between VOC measurements (50ms - 5s)
   * @return True if successful
   */
  bool setMPPTVOCDelay(modbee_voc_delay_t delay);
  
  /*!
   * @brief Set VOC measurement rate
   * @param rate Rate of VOC measurements (30s - 30min)
   * @return True if successful
   */
  bool setMPPTVOCRate(modbee_voc_rate_t rate);
  
  /*!
   * @brief Convert VOC percentage enum to actual percentage value
   * @param voc_enum VOC percentage enum value
   * @return Actual percentage as float (56.25 - 100.0)
   */
  static float vocPercentToFloat(modbee_voc_percent_t voc_enum);
  
  /*!
   * @brief Convert actual percentage to closest VOC percentage enum
   * @param percentage Actual percentage (56.25 - 100.0)
   * @return Closest VOC percentage enum value
   */
  static modbee_voc_percent_t floatToVocPercent(float percentage);

  /*!
   * @brief Get all charger status registers (0-4)
   * @param status0 Pointer to store Status 0 (power/input status)
   * @param status1 Pointer to store Status 1 (charging status)
   * @param status2 Pointer to store Status 2 (DPDM status)
   * @param status3 Pointer to store Status 3 (ICO/thermal/ADC status)
   * @param status4 Pointer to store Status 4 (system status)
   */
  void getAllChargerStatus(uint8_t* status0, uint8_t* status1, uint8_t* status2, 
                          uint8_t* status3, uint8_t* status4);
  
  /*!
   * @brief Get input power management status
   * @return True if in VINDPM or IINDPM
   */
  bool isInInputPowerManagement();
  
  /*!
   * @brief Check if power is good (VBUS present and valid)
   * @return True if power is good
   */
  bool isPowerGood();
  
  /*!
   * @brief Check if VBUS is present
   * @return True if VBUS is present
   */
  bool isVbusPresent();
  
  /*!
   * @brief Check if battery is present
   * @return True if battery is detected
   */
  bool isBatteryPresent();
  
  /*!
   * @brief Check if in thermal regulation
   * @return True if thermal regulation is active
   */
  bool isInThermalRegulation();
  
  /*!
   * @brief Check if ICO (Input Current Optimizer) is complete
   * @return True if ICO optimization is complete
   */
  bool isICOComplete();
  
  /*!
   * @brief Check if ADC conversion is complete
   * @return True if ADC conversion is done
   */
  bool isADCComplete();

  /*!
   * @brief Get Status 0 register decoded as string
   * @return Status 0 decoded (IINDPM, VINDPM, WD, etc.)
   */
  String getStatus0String();
  
  /*!
   * @brief Get Status 1 register decoded as string 
   * @return Status 1 decoded (charge state + VBUS status)
   */
  String getStatus1String();
  
  /*!
   * @brief Get Status 2 register decoded as string
   * @return Status 2 decoded (DPDM detection result)
   */
  String getStatus2String();
  
  /*!
   * @brief Get Status 3 register decoded as string
   * @return Status 3 decoded (ICO, thermal, ADC status)
   */
  String getStatus3String();
  
  /*!
   * @brief Get Status 4 register decoded as string
   * @return Status 4 decoded (system status)
   */
  String getStatus4String();
  
  /*!
   * @brief Determine battery current direction
   * @return "Charging", "Discharging", or "Idle"
   */
  String getBatteryCurrentDirection();

  // ========================================================================
  // BATTERY PROTECTION AND SAFETY FUNCTIONS
  // ========================================================================
  
  /*!
   * @brief Set battery discharge current limit for protection
   * @param current Discharge current limit (0.0=disable, 3.0A, 4.0A, or 5.0A)
   * @return True if successful
   */
  bool setBatteryDischargeCurrentLimit(float current);
  
  /*!
   * @brief Enable/disable battery discharge overcurrent protection
   * @param enable True to enable OCP protection
   * @return True if successful
   */
  bool setBatteryDischargeOCPEnable(bool enable);
  
  /*!
   * @brief Set input overvoltage protection threshold
   * @param threshold OVP threshold (7V, 12V, 22V, or 26V)
   * @return True if successful
   */
  bool setInputOVPThreshold(float threshold);
  
  /*!
   * @brief Check if battery voltage is below safe discharge level
   * @return True if battery needs protection from over-discharge
   */
  bool isBatteryLowVoltage();
  
  /*!
   * @brief Check if system voltage is at minimum level
   * @return True if system is at minimum voltage (load shedding may occur)
   */
  bool isSystemAtMinVoltage();

  // ========================================================================
  // STRUCTURED STATUS ACCESS (Simple approach with datasheet-accurate structs)
  // ========================================================================
  
  /*!
   * @brief Get complete device status in structured format
   * @return Complete status structure with all register data
   */
  modbee_complete_status_t getCompleteStatus();
  
  /*!
   * @brief Get Status Register 0 (REG1B)
   * @return Status0 structure with all bits decoded
   */
  modbee_status0_t getStatus0();
  
  /*!
   * @brief Get Status Register 1 (REG1C)
   * @return Status1 structure with charge state and VBUS status
   */
  modbee_status1_t getStatus1();
  
  /*!
   * @brief Get Status Register 2 (REG1D)
   * @return Status2 structure with ICO, thermal, and battery status
   */
  modbee_status2_t getStatus2();
  
  /*!
   * @brief Get Status Register 3 (REG1E)
   * @return Status3 structure with timers and system status
   */
  modbee_status3_t getStatus3();
  
  /*!
   * @brief Get Status Register 4 (REG1F)
   * @return Status4 structure with temperature status
   */
  modbee_status4_t getStatus4();
  
  /*!
   * @brief Get Fault Register 0 (REG20)
   * @return Fault0 structure with protection status
   */
  modbee_fault0_t getFault0();
  
  /*!
   * @brief Get Fault Register 1 (REG21)
   * @return Fault1 structure with system faults
   */
  modbee_fault1_t getFault1();

  // Peak and total energy tracking
  float getVin1PeakPower() const;
  float getVin1TotalEnergyWh() const;
  void resetVin1Stats();

  float getVin2PeakPower() const;
  float getVin2TotalEnergyWh() const;
  void resetVin2Stats();

  float getVbusPeakPower() const;
  float getVbusTotalEnergyWh() const;
  void resetVbusStats();

  float getBatteryPeakPower() const;
  float getBatteryTotalEnergyWh() const;
  void resetBatteryStats();

  float getSystemPeakPower() const;
  float getSystemTotalEnergyWh() const;
  void resetSystemStats();

  // Battery stats tracking
  float _batteryPeakChargeAmps = 0, _batteryPeakDischargeAmps = 0;
  float _batteryAmpHoursCharge = 0, _batteryAmpHoursDischarge = 0;
  float _batteryPeakDischargePower = 0;
  float _batteryWattHoursDischarge = 0;

  // Battery stats methods
  float getBatteryPeakChargeAmps() const;
  float getBatteryPeakDischargeAmps() const;
  float getBatteryAmpHoursCharge() const;
  float getBatteryAmpHoursDischarge() const;
  float getBatteryPeakDischargePower() const;
  float getBatteryWattHoursDischarge() const;
  void resetBatteryAmpStats();
  void resetBatteryDischargePowerStats();

  // Setters for restoring stats
  void setVin1PeakPower(float p);
  void setVin1TotalEnergyWh(float e);

  void setVin2PeakPower(float p);
  void setVin2TotalEnergyWh(float e);

  void setVbusPeakPower(float p);
  void setVbusTotalEnergyWh(float e);

  void setBatteryPeakPower(float p);
  void setBatteryTotalEnergyWh(float e);

  void setSystemPeakPower(float p);
  void setSystemTotalEnergyWh(float e);

  void setBatteryPeakChargeAmps(float a);
  void setBatteryPeakDischargeAmps(float a);
  void setBatteryAmpHoursCharge(float ah);
  void setBatteryAmpHoursDischarge(float ah);
  void setBatteryPeakDischargePower(float p);
  void setBatteryWattHoursDischarge(float wh);

private:
  ModbeeMPPT& _mppt;  // Reference to the ModbeeMPPT instance
  
  // Battery configuration for SoC calculation
  float _battery_min_voltage;
  float _battery_max_voltage;
  float _battery_nominal_voltage;
  modbee_battery_type_t _battery_type;
  uint8_t _battery_cell_count;
  
  // True battery voltage state machine
  enum TrueBatteryVoltageState {
    TBVS_IDLE,
    TBVS_DISABLE_CHARGING,
    TBVS_WAIT_CHARGE_STOP,
    TBVS_FORCE_DISCHARGE,
    TBVS_WAIT_STABILIZE,
    TBVS_READ_VOLTAGE,
    TBVS_WAIT_BEFORE_RESTORE,
    TBVS_RESTORE
  };
  
  TrueBatteryVoltageState _tbv_state;
  unsigned long _tbv_timer;
  bool _tbv_original_charge_state;
  bool _tbv_original_discharge_state;
  float _tbv_last_reading;
  bool _tbv_reading_valid;
  
  // Stats tracking
  float _vin1PeakPower = 0, _vin1TotalEnergyWh = 0;
  float _vin2PeakPower = 0, _vin2TotalEnergyWh = 0;
  float _vbusPeakPower = 0, _vbusTotalEnergyWh = 0;
  float _batteryPeakPower = 0, _batteryTotalEnergyWh = 0;
  float _systemPeakPower = 0, _systemTotalEnergyWh = 0;
  unsigned long _lastStatsUpdateMs = 0;
  
  // Helper functions
  float clampValue(float value, float min_val, float max_val);
  float calculateBatterySOC(float voltage);
};

#endif // MODBEE_MPPT_API_H
