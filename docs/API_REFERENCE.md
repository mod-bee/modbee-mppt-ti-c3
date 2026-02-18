# ModbeeMpptAPI - Complete Reference

Real API reference for `ModbeeMpptAPI` class. All methods from actual codebase.

## ⚡ Quick Method Reference

### Power & Energy
| Method | Returns | Description |
|--------|---------|-------------|
| `updateStats()` | void | Update peak power and total energy statistics |
| `getVbusPower()` | `modbee_power_data_t` | Solar input power (voltage × current) |
| `getBatteryPower()` | `modbee_power_data_t` | Battery power (positive=charge, negative=discharge) |
| `getSystemPower()` | `modbee_power_data_t` | System rail power from power balance equation |
| `getVAC1Power()` | `modbee_power_data_t` | VAC1 input power |
| `getVAC2Power()` | `modbee_power_data_t` | VAC2 input power |
| `getEfficiency()` | float | System efficiency (%) |

### Voltage & Current
| Method | Returns | Description |
|--------|---------|-------------|
| `getVbusVoltage()` | float | Solar input voltage (V) |
| `getIbusCurrent()` | float | Solar input current (A) |
| `getBatteryVoltage()` | float | Battery terminal voltage (V) |
| `getBatteryCurrent()` | float | Battery charging current (A) |
| `getSystemVoltage()` | float | System rail voltage (V) |
| `getSystemCurrent()` | float | System load current (A) |
| `getVAC1Voltage()` | float | VAC1 voltage (V) |
| `getVAC2Voltage()` | float | VAC2 voltage (V) |

### Battery Monitoring
| Method | Parameters | Returns | Description |
|--------|-----------|---------|-------------|
| `getBatteryChargePercent()` | - | float | Battery SOC (0-100%) |
| `getBatteryTemperature()` | - | float | Battery temperature (°C) |
| `getRawTSPercent()` | - | float | Raw TS ADC reading (0-100%) |
| `updateTrueBatteryVoltage()` | - | void | Start true voltage measurement |
| `getTrueBatteryVoltage()` | - | float | True battery voltage (no-load) |
| `getBatteryChargingVoltage()` | - | float | Terminal voltage while charging |
| `getActualBatterySOC()` | - | float | SOC based on full range (0-100%) |
| `getUsableBatterySOC()` | - | float | SOC based on operating range (0-100%) |
| `getComprehensiveBatteryStatus()` | - | `modbee_battery_status_t` | Complete battery snapshot |
| `setBatteryType()` | type, cell_count | bool | Configure battery chemistry |
| `setBatteryVoltageRange()` | min_voltage, max_voltage | bool | Set custom voltage range |

### Charging Control
| Method | Parameters | Returns | Description |
|--------|-----------|---------|-------------|
| `setChargeVoltage()` | voltage (3.0-18.8V) | bool | Set target charge voltage |
| `setChargeCurrent()` | current (0.0-5.0A) | bool | Set maximum charge current |
| `getChargeVoltage()` | - | float | Get charge voltage setpoint (V) |
| `getChargeCurrent()` | - | float | Get charge current limit (A) |
| `getTerminationCurrent()` | - | float | Get end-of-charge threshold (A) |
| `setTerminationCurrent()` | current | bool | Set end-of-charge threshold |
| `getRechargeThreshold()` | - | float | Get recharge threshold offset |
| `setRechargeThreshold()` | offset | bool | Set recharge threshold |
| `getPrechargeCurrent()` | - | float | Get precharge current (A) |
| `setPrechargeCurrent()` | current | bool | Set precharge current |
| `getPrechargeVoltageThreshold()` | - | `modbee_vbat_lowv_t` | Get precharge exit voltage |
| `setPrechargeVoltageThreshold()` | threshold | bool | Set precharge exit voltage |
| `setChargeEnable()` | enable | bool | Enable/disable charging |
| `getChargeEnable()` | - | bool | Check if charging enabled |

### Input Limits
| Method | Parameters | Returns | Description |
|--------|-----------|---------|-------------|
| `setInputCurrentLimit()` | current | bool | Set max input current |
| `setInputVoltageLimit()` | voltage | bool | Set max input voltage |
| `getInputVoltageLimit()` | - | float | Get input voltage limit (V) |
| `getInputCurrentLimit()` | - | float | Get input current limit (A) |
| `setMinSystemVoltage()` | voltage | bool | Set VSYS floor |
| `getMinSystemVoltage()` | - | float | Get min system voltage (V) |
| `setVACOVP()` | voltage | bool | Set VAC overvoltage threshold |
| `getVACOVP()` | - | float | Get VAC OVP threshold (V) |
| `getCellCount()` | - | uint8_t | Get configured cell count |

### MPPT Control
| Method | Parameters | Returns | Description |
|--------|-----------|---------|-------------|
| `setMPPTEnable()` | enable | bool | Enable/disable MPPT |
| `getMPPTEnable()` | - | bool | Check if MPPT enabled |
| `setMPPTVOCPercent()` | percent | bool | Set MPPT VOC reference (%) |
| `getMPPTVOCPercent()` | - | `modbee_voc_percent_t` | Get current VOC setting |
| `setMPPTVOCDelay()` | delay | bool | Set tracking delay interval |
| `setMPPTVOCRate()` | rate | bool | Set tracking adjustment rate |

### ADC Control
| Method | Parameters | Returns | Description |
|--------|-----------|---------|-------------|
| `setADCEnable()` | enable | bool | Enable/disable ADC |
| `getADCEnable()` | - | bool | Check if ADC enabled |
| `setADCMode()` | mode | bool | Set mode (CONTINUOUS/ONE_SHOT) |
| `setADCAveraging()` | averaging | bool | Set averaging (1,2,4,8 samples) |
| `setADCResolution()` | resolution | bool | Set resolution (12-15 bit) |
| `configureADC()` | resolution, averaging, mode | bool | Configure all ADC settings |
| `isADCConversionDone()` | - | bool | Check if conversion complete |
| `getADCControlRegister()` | - | uint8_t | Get raw ADC control register |

### Timers
| Method | Parameters | Returns | Description |
|--------|-----------|---------|-------------|
| `setFastChargeTimer()` | timer | bool | Set fast-charge timeout |
| `setFastChargeTimerEnable()` | enable | bool | Enable/disable fast-charge timer |
| `getFastChargeTimerEnable()` | - | bool | Check if timer enabled |
| `setPrechargeTimer()` | timer | bool | Set precharge timeout |
| `setPrechargeTimerEnable()` | enable | bool | Enable/disable precharge timer |
| `getPrechargeTimerEnable()` | - | bool | Check if timer enabled |
| `getTopOffTimer()` | - | `modbee_topoff_timer_t` | Get top-off timer setting |
| `setTopOffTimer()` | timer | bool | Set top-off timer |
| `setTrickleChargeTimerEnable()` | enable | bool | Enable/disable trickle timer |
| `getTrickleChargeTimerEnable()` | - | bool | Check if trickle enabled |
| `setTimerHalfRateEnable()` | enable | bool | Enable timer half-rate mode |
| `getTimerHalfRateEnable()` | - | bool | Check if half-rate enabled |

### System Control
| Method | Parameters | Returns | Description |
|--------|-----------|---------|-------------|
| `setWatchdogEnable()` | enable | bool | Enable/disable watchdog |
| `setWatchdogTimer()` | timer | bool | Set timeout (DISABLE/40s/80s/160s) |
| `resetWatchdog()` | - | bool | Reset watchdog timer |
| `getWatchdogEnable()` | - | bool | Check if watchdog enabled |
| `getWatchdogTimer()` | - | `modbee_watchdog_timer_t` | Get timer setting |
| `setPWMFrequency()` | frequency | bool | Set PWM switching frequency |
| `getPWMFrequency()` | - | `modbee_pwm_freq_t` | Get frequency setting |
| `setForwardPFM()` | enable | bool | Enable pulse frequency modulation |
| `getForwardPFM()` | - | bool | Check if PFM enabled |
| `setForwardOOA()` | enable | bool | Enable one-wire forward |
| `getForwardOOA()` | - | bool | Check if OOA enabled |
| `setHIZMode()` | enable | bool | Enable high-impedance mode |
| `getHIZMode()` | - | bool | Check if HIZ enabled |
| `setBackupMode()` | enable | bool | Enable backup mode |
| `getBackupMode()` | - | bool | Check if backup enabled |
| `setShippingMode()` | enable | bool | Enter shipping mode |
| `getShipMode()` | - | bool | Check if in ship mode |
| `getDieTemperature()` | - | float | Get BQ25798 die temp (°C) |
| `detectBatteryConnected()` | - | bool | Detect battery connection |
| `setBatteryDischargeSenseEnable()` | enable | bool | Enable discharge sensing |
| `getBatteryDischargeSenseEnable()` | - | bool | Check if sensing enabled |
| `setICOEnable()` | enable | bool | Enable input current optimizer |

### Status & Faults
| Method | Returns | Description |
|--------|---------|-------------|
| `getChargeStateString()` | String | Get phase name ("Bulk", "Float", etc.) |
| `isCharging()` | bool | Check if actively charging |
| `hasFaults()` | bool | Check for any fault condition |
| `getFaultString()` | String | Get fault description |
| `getAllFaultStatus()` | void | Get raw fault registers |
| `hasInputOvervoltageFault()` | bool | Check input OVP fault |
| `hasOvercurrentFault()` | bool | Check overcurrent fault |
| `hasThermalFault()` | bool | Check thermal fault |

### Utilities
| Method | Parameters | Returns | Description |
|--------|-----------|---------|-------------|
| `vocPercentToFloat()` | voc_enum | float | Convert VOC enum to % |
| `floatToVocPercent()` | percentage | `modbee_voc_percent_t` | Convert % to VOC enum |
| `update()` | - | void | Internal state machine (call via loop) |

---

## 📋 Data Structures

### `modbee_power_data_t`
```cpp
struct {
  float voltage;     // Volts
  float current;     // Amps
  float power;       // Watts
  bool valid;        // Data validity
};
```

### `modbee_battery_status_t`
```cpp
struct {
  float charging_voltage;      // Terminal voltage while charging
  float true_voltage;          // Measured no-load voltage
  float actual_soc;            // SOC 0-100% (full range)
  float usable_soc;            // SOC 0-100% (operating range)
};
```

---

## 💡 Common Examples

### Basic Setup
```cpp
void setup() {
  mppt.begin();
  mppt.api.setBatteryType(MODBEE_BATTERY_LIPO, 3);
  mppt.api.setChargeVoltage(12.6);
  mppt.api.setChargeCurrent(2.0);
  mppt.api.setMPPTEnable(true);
}
```

### Real-Time Monitoring
```cpp
void loop() {
  mppt.loop();
  
  auto vbus = mppt.api.getVbusPower();
  auto vbat = mppt.api.getBatteryPower();
  Serial.printf("Solar: %.1fW | Batt: %.1fW | SOC: %.1f%%\n",
                vbus.power, vbat.power, mppt.api.getBatteryChargePercent());
}
```

### Thermal Derating
```cpp
float temp = mppt.api.getBatteryTemperature();
if (temp > 50.0f) mppt.api.setChargeCurrent(0.5);
else if (temp > 40.0f) mppt.api.setChargeCurrent(1.0);
else mppt.api.setChargeCurrent(2.0);
```

### Battery Status Snapshot
```cpp
auto status = mppt.api.getComprehensiveBatteryStatus();
Serial.printf("Charging: %.2fV | True: %.2fV | SOC: %.1f%% (usable)\n",
              status.charging_voltage, status.true_voltage, status.usable_soc);
```

### Power Efficiency Check
```cpp
auto input = mppt.api.getVbusPower();
if (input.valid) {
  Serial.printf("System efficiency: %.1f%%\n", mppt.api.getEfficiency());
}
```

---

**This is the real API.** All methods from ModbeeMpptAPI.cpp. No fabrication.
