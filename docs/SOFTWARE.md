# ModBee MPPT Charger - Software & Firmware Guide

Firmware architecture, configuration management, and customization guide for the ModBee MPPT Charger.

## 📦 Firmware Architecture

The firmware is built around a simple, single `ModbeeMPPT` class that handles all functionality:

```
┌─────────────────────────────────────────────────────┐
│         Main Application (src/main.cpp)             │
│              ModbeeMPPT modbeeMPPT;                 │
│         setup() → begin()                           │
│         loop() → loop()                             │
└────────────────────┬────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────┐
│      ModbeeMPPT Class (lib/ModbeeMPPT)              │
│                                                     │
│  ├─ Manages BQ25798 via I2C                         │
│  ├─ Battery detection & charging control            │
│  ├─ MPPT tracking (autonomous VOC)                  │
│  ├─ WiFi AP mode (ModbeeMPPT SSID)                  │
│  ├─ Web server (http://192.168.4.1)                 │
│  ├─ Configuration (JSON, LittleFS)                  │
│  └─ Status LEDs & indicators                        │
└────────────────────┬────────────────────────────────┘
                     │
    ┌────────────────┼────────────────┐
    │                │                │
┌───▼──────┐    ┌────▼─────┐   ┌──────▼───────┐
│BQ25798   │    │LittleFS  │   │ESP32 WiFi    │
│(I2C)     │    │Config    │   │WebSocket     │
└──────────┘    └──────────┘   └──────────────┘
```

## 🚀 Main Application

The user application is extremely simple:

```cpp
#include "ModbeeMpptGlobal.h"
#include "ModbeeMPPT.h"

ModbeeMPPT modbeeMPPT;

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("=== ModbeeMPPT API Example ===");
  modbeeMPPT.initializeLEDs();  // Setup LED indicator
  modbeeMPPT.begin();           // Initialize everything
  modbeeMPPT.initWebServer();   // Start web interface
  Serial.println("Ready!");
}

void loop() {
  modbeeMPPT.loop();  // Main update - handles everything
  
  // Optional: print status every 2 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    modbeeMPPT.printStatus();  // Show detailed status
  }
}
```

That's it! The `ModbeeMPPT` class handles all functionality internally.

## 📦 ModbeeMPPT Class API

```cpp
class ModbeeMPPT {
public:
  // Core methods
  ModbeeMPPT();
  bool begin(const char* configFile = nullptr);  // Initialize
  void loop();                                    // Call continuously
  
  // LED Management
  void initializeLEDs(int numLeds = 1, uint8_t brightness = 250);
  void updateLEDs();  // Called automatically
  
  // Web Server
  void initWebServer();
  void enableWebServer();
  bool isWebServerEnabled();
  
  // Status & Diagnostics (output to Serial)
  void printStatus();                          // Complete status
  void printQuickStatus();                     // Essentials only
  void printPowerMeasurements();               // Voltage/current
  void printConfiguration();                   // Current settings
  void printFaults();                         // Error status
  void printRegisterDebug();                  // Raw BQ25798 registers
  void printComprehensiveBatteryStatus();     // Detailed battery info
  
  // Battery Management
  void enableChargingIfBatteryPresent();
  
  // Configuration
  bool saveConfig();
  bool loadConfig();
  bool resetConfig();
  
  // Status Queries
  bool hasFaults();
  String getChargeStateString();  // Returns "Bulk", "Float", etc.
};
```

## ⚙️ Configuration System

Configuration is stored as JSON in `/config/mppt_config.json` on the device's LittleFS filesystem.

### Example Config File

```json
{
  "battery": {
    "type": 0,
    "cell_count": 3,
    "charge_voltage": 12.6,
    "charge_current": 1.0,
    "min_system_voltage": 10.0
  },
  "charging": {
    "termination_current": 0.12,
    "recharge_threshold": 0.4,
    "precharge_current": 0.2,
    "precharge_voltage_threshold": 3
  },
  "input": {
    "voltage_limit": 22.0,
    "current_limit": 3.0,
    "vac_ovp_threshold": 26.0
  },
  "timers": {
    "fast_charge_enable": true,
    "fast_charge_timer": 5,
    "precharge_enable": true,
    "precharge_timer": 1,
    "topoff_timer": 1
  },
  "mppt": {
    "voc_percent": 0,
    "voc_delay": 1,
    "voc_rate": 0,
    "enable": true
  },
  "intervals": {
    "battery_check": 10000,
    "soc_check": 30000
  }
}
```

### Accessing Configuration

```cpp
// Read configuration
modbee_battery_type_t type = modbeeMPPT.config.data.battery_type;
float chargeVolts = modbeeMPPT.config.data.charge_voltage;
float chargeCurrent = modbeeMPPT.config.data.charge_current;

// Modify and save
modbeeMPPT.config.data.charge_current = 2.0f;  // 2A
modbeeMPPT.config.saveConfig();  // Persist to device

// Reload from file
modbeeMPPT.config.loadConfig();
```

### Battery Types

```cpp
enum {
  MODBEE_BATTERY_LIFEPO4 = 0,
  MODBEE_BATTERY_LIPO = 1,
  MODBEE_BATTERY_LEAD_ACID = 2,
  MODBEE_BATTERY_CUSTOM = 3
};
```

## 🔌 ModbeeMpptAPI - Safe Interface to BQ25798

The `ModbeeMpptAPI` class provides safe, validated access to the BQ25798 charger. You don't access I2C directly.

### Key Methods

```cpp
class ModbeeMpptAPI {
public:
  // Charging control
  void setChargeVoltage(float voltage_V);    // 3.0 - 18.8V
  void setChargeCurrent(float current_A);    // 0.0 - 5.0A
  void enableCharging();
  void disableCharging();
  bool isCharging();
  
  // Power readings (in real units)
  float getInputVoltage();      // Solar input (V)
  float getInputCurrent();      // Solar current (A)
  float getBatteryVoltage();    // Battery (V)
  float getChargeCurrent();     // Charge current (A)
  float getSystemVoltage();     // SYS rail (V)
  float getSystemCurrent();     // Load current (A)
  
  // Battery monitoring
  bool isBatteryDetected();
  float getBatterySOC();        // State of charge (0-100%)
  float getBatteryTemperature();// From NTC sensor (°C)
  
  // MPPT control
  void enableMPPT();
  void disableMPPT();
  void setMPPT_VOC_Percent(modbee_voc_percent_t percent);
  
  // Status & faults
  ChargePhase getChargePhase();  // PRECHARGE, BULK, ABSORPTION, FLOAT, IDLE
  uint8_t getChargeStatus();
  bool hasFault();
  String getFaultString();
};
```

### Example: Custom Charging Logic

```cpp
void customChargeLogic() {
  float vbat = modbeeMPPT.api.getBatteryVoltage();
  float temp = modbeeMPPT.api.getBatteryTemperature();
  
  // Thermal de-rating: reduce charge at high temp
  if (temp > 50.0f) {
    Serial.printf("Thermal limit! Temp=%.1f°C\n", temp);
    modbeeMPPT.api.setChargeCurrent(0.5);  // Reduce to 0.5A
  }
  
  // Detect full charge
  if (modbeeMPPT.api.getChargePhase() == PHASE_FLOAT) {
    Serial.println("Battery fully charged");
  }
  
  // Show SOC
  Serial.printf("Battery SOC: %.1f%%\n", modbeeMPPT.api.getBatterySOC());
}

void loop() {
  modbeeMPPT.loop();
  customChargeLogic();
}
```

## 🌐 Web Interface

### Auto-Starting WiFi AP

The device automatically starts a WiFi access point on boot:
- **SSID**: `ModbeeMPPT`
- **Password**: None (open network)
- **IP**: `http://192.168.4.1`
- **DNS**: Redirects all domains to 192.168.4.1

### Web Files

Served from `/data/` directory (uploaded via filesystem upload):
- `index.html` - Real-time dashboard
- `settings.html` - Configuration UI
- `debug.html` - Diagnostics

### WebSocket Data

Dashboard receives real-time updates via WebSocket `/ws`:

```json
{
  "vbus": 18200,      // mV
  "ibus": 450,        // mA
  "vbat": 12400,      // mV
  "ibat": 1200,       // mA
  "vsys": 12000,      // mV
  "isys": 50,         // mA
  "temp": 28,         // °C
  "soc": 75.5,        // %
  "phase": "Bulk"     // Charging phase
}
```

## 🔋 Charging Phases

Automatically managed by BQ25798:

```cpp
enum ChargePhase {
  PHASE_PRECHARGE = 0,
  PHASE_BULK = 1,
  PHASE_ABSORPTION = 2,
  PHASE_FLOAT = 3,
  PHASE_IDLE = 4
};
```

| Phase | Behavior | Condition |
|-------|----------|-----------|
| Precharge | ~0.2A until safe voltage | Cold or deeply discharged |
| Bulk | Maximum current | 0-80% SOC |
| Absorption | Constant voltage, decreasing I | 80-95% SOC |
| Float | Trickle charge 10-50mA | 95%+ SOC |
| Idle | No charging | Battery full |

## 🛠️ Build & Upload

### In VS Code PlatformIO

```bash
# Build firmware
pio run -e lolin_c3_mini

# Upload firmware
pio run -e lolin_c3_mini -t upload

# Upload web files (filesystem)
pio run -e lolin_c3_mini -t uploadfs

# Monitor serial output
pio device monitor --baud 115200
```

### Environment

File: `platformio.ini`
```ini
[env:lolin_c3_mini]
platform = espressif32
board = lolin_c3_mini
framework = arduino
board_build.filesystem = littlefs
```

## 📝 Common Customizations

### Change WiFi SSID/Password

Edit `lib/ModbeeMPPT/src/ModbeeMpptWebServer.cpp`:

```cpp
#define WIFI_SSID "MySSID"
#define WIFI_PASSWORD "MyPassword"
```

### Adjust Update Intervals

```cpp
// Battery check every 5 seconds
modbeeMPPT.config.data.battery_check_interval = 5000;

// SOC check every 60 seconds
modbeeMPPT.config.data.soc_check_interval = 60000;

modbeeMPPT.config.saveConfig();
```

### Add Custom Debug Output

```cpp
void loop() {
  modbeeMPPT.loop();
  
  // Print SOC every 30 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 30000) {
    lastPrint = millis();
    Serial.printf("SOC: %.1f%%\n", modbeeMPPT.api.getBatterySOC());
  }
}
```

### Disable MPPT

```cpp
modbeeMPPT.config.data.mppt_enable = false;
modbeeMPPT.config.saveConfig();
```

## 🐛 Debugging

### Print Status

```cpp
modbeeMPPT.printStatus();  // Full comprehensive status
```

Output example:
```
=== MPPT System Status ===
Input:   VBUS=18.2V, IBUS=0.45A, Power=8.19W
Battery: VBAT=12.4V, IBAT=1.2A, SOC=75.5%, Temp=28°C
System:  VSYS=12.0V, ISYS=50mA
Phase: Bulk Charge
Config: 3S Li-Ion, 1.0A limit, MPPT enabled
```

### Check Faults

```cpp
modbeeMPPT.printFaults();

// Or programmatically
if (modbeeMPPT.api.hasFault()) {
  Serial.println(modbeeMPPT.api.getFaultString());
}
```

### Raw Register Dump

```cpp
modbeeMPPT.printRegisterDebug();  // All BQ25798 registers
```

## 📂 File Structure

```
modbee-mppt-ti-c3/
├── src/
│   └── main.cpp ........................ Simple user app
├── lib/ModbeeMPPT/src/
│   ├── ModbeeMPPT.h/cpp ............ Main controller
│   ├── ModbeeMpptAPI.h/cpp ........ I2C interface to BQ25798
│   ├── ModbeeMpptConfig.h/cpp ..... JSON configuration
│   ├── ModbeeMpptWebServer.h/cpp .. WiFi & web interface
│   ├── ModbeeMpptDebug.h/cpp ...... Debug output functions
│   └── ModbeeMpptGlobal.h/cpp .... Global definitions
├── data/
│   ├── index.html ................. Dashboard
│   ├── settings.html .............. Configuration UI
│   └── debug.html ................. Diagnostics
├── lib/
│   ├── bq25798/ ................... TI BQ25798 driver
│   ├── ArduinoJson/ ............... JSON library
│   ├── ESPAsyncWebServer/ ......... Async web server
│   └── ... (other libraries)
└── platformio.ini ................. Build config
```

## 🔑 Key Concepts

1. **Simple API** - Single `ModbeeMPPT` class handles everything
2. **JSON Configuration** - Persisted to LittleFS, editable via web UI
3. **No Raw I2C** - Use `ModbeeMpptAPI` methods only
4. **Autonomous MPPT** - BQ25798 handles VOC tracking automatically
5. **Web-First** - Browser access at http://192.168.4.1/
6. **Auto-Detect Battery** - Starts charging when battery detected

## � Upcoming Features

### Modbus Telemetry (Coming Soon)

Modbus RTU telemetry interface for remote monitoring and integration with other systems. Allows real-time data streaming of voltage, current, SOC, temperature, and charging phase via Modbus protocol.

**Expected Features:**
- Modbus RTU over serial/RS485
- Real-time power measurements
- Battery state parameters
- Charging phase reporting
- Configurable polling intervals

### I2C Slave Telemetry (Coming Soon)

I2C slave interface for integration with master controllers (e.g., Victron Cerbo GX, custom controllers). Device acts as an I2C slave, allowing external systems to query system state in real-time.

**Expected Features:**
- I2C slave address configuration
- Register-based data access
- Interrupt signaling on state changes
- Multi-master support

## 🤝 Contributing

Interested in contributing? Here are some areas actively being developed:

- **Modbus RTU Implementation** - Help implement the Modbus telemetry interface
- **I2C Slave Interface** - Develop I2C slave protocol and integration tests
- **Web UI Enhancements** - Improve dashboard, add more analytics
- **Battery Profile Library** - Add support for more battery types and custom profiles
- **Performance Optimization** - Improve MPPT tracking efficiency
- **Documentation** - Help improve guides and examples

See the main [README.md](../README.md) for contribution guidelines.

## �📞 Support

**Firmware won't compile?**  
→ Check `platformio.ini` and library paths in `lib/`

**Web interface not loading?**  
→ Re-upload filesystem with `pio run -e lolin_c3_mini -t uploadfs`

**Battery not detected?**  
→ Check voltage (>3V), polarity, and battery type setting

**Issues?** See [docs/HARDWARE.md](./HARDWARE.md) for electrical problems
