# Getting Started with ModBee MPPT Charger

This guide walks you through setup, programming, and deployment of your ModBee MPPT Charger.

## 📋 Prerequisites

### Hardware
- ModBee MPPT Charger board (LOLIN C3 Mini)
- USB-C cable (for programming)
- Solar panel (3.6V - 24V input recommended)
- Rechargeable battery (1-4 cell Li-Ion, Li-Polymer, or LiFePO4)

### Software
- **Visual Studio Code** with PlatformIO IDE extension
- **Python 3.6+** (installed automatically with PlatformIO)
- **Git** (to clone the repository)

## 💻 Installation & Setup

### Step 1: Install Visual Studio Code + PlatformIO

1. **Install VS Code**: https://code.visualstudio.com/
2. **Install PlatformIO IDE Extension**:
   - Open VS Code
   - Go to Extensions (left sidebar)
   - Search "PlatformIO IDE"
   - Click Install

### Step 2: Clone the Repository

```bash
git clone https://github.com/mod-bee/modbee-mppt-ti-c3.git
cd modbee-mppt-ti-c3
code .  # Open in VS Code
```

### Step 3: Connect Hardware & Build

**Physical Connections** (before power):
- Connect solar panel to **J1 (PV+/PV-)**
- Connect battery to **J2 (BAT+/BAT-)**
- Connect USB-C to computer

**Upload Firmware**:

In VS Code:
1. Open PlatformIO
2. Open project and select mppt project
3. Click **Upload** (arrow icon)
   - Firmware compiles and uploads
   - Progress shown in terminal

```bash
# Or via terminal:
pio run -e lolin_c3_mini -t upload
```

### Step 4: Upload Web Interface Files

The HTML/CSS/JS files for the web dashboard are stored in `/data/` and need to be uploaded to the device's LittleFS filesystem.

In VS Code PlatformIO:
1. Click **Upload Filesystem Image** (in PlatformIO sidebar)
   - Files from `data/` directory upload to device storage
   - Progress shown in terminal

```bash
# Or via terminal:
pio run -e lolin_c3_mini -t uploadfs
```

**⚠️ Important**: Upload filesystem **after** firmware upload for first-time setup.

### Step 5: Monitor Serial Output

In VS Code:
1. Click **Serial Monitor** in PlatformIO sidebar
2. Baud rate: **115200**
3. Watch for boot messages:

```
=== ModbeeMPPT API Example ===
MPPT controller initialized and configured successfully!
Web server should start automatically...
```

## 🌐 Access Web Interface

### First Boot
1. The device starts WiFi AP (access point) automatically
2. Look for WiFi network: **ModbeeMPPT**
3. Connect with any device (phone, laptop)
   - Network is open (no password)
4. Open browser to: **http://192.168.4.1/**
   - Dashboard loads showing real-time charging status
   - Three pages available:
     - **index.html** - Main dashboard (voltage, current, power, SOC)
     - **settings.html** - Configuration (battery type, charge limits, MPPT)
     - **debug.html** - Diagnostics (register dumps, power measurements)

### Configuration via Web UI

**Settings Page** provides control over:
- Battery type (Li-Ion, LiFePO4, Li-Po, Lead-Acid)
- Cell count (1-4 cells)
- Charge voltage & current
- Input voltage/current limits
- MPPT settings (VOC%, tracking delay)

Changes are saved to `/config/mppt_config.json` on the device and persist after restart.

## 🚀 First-Time Operation

### Initial Setup Checklist

✅ Firmware uploaded  
✅ Filesystem uploaded (web files)  
✅ USB-C connected for power  
✅ Solar panel connected  
✅ Battery connected

### What Happens on Boot

1. **LED Indicator**:
   - Green LED turns on ✓ (MCU ready)
   - Device enters WiFi AP mode

2. **Web Interface Available**:
   - Connect to WiFi: **ModbeeMPPT**
   - Visit: **http://192.168.4.1/**

3. **Auto-Detection**:
   - Device automatically detects connected battery
   - Checks solar panel input voltage
   - Initializes BQ25798 charger
   - Starts charging if battery detected and solar available

### Monitor Device Status

**Via Serial Monitor** (115200 baud):
```
=== ModbeeMPPT API Example ===
MPPT controller initialized and configured successfully!
[Complete comprehensive status]
Input: 18.2V @ 0.45A
Battery: 12.4V @ 1.2A (Charging - Bulk)
System: 12.0V @ 0.05A
```

**Via Web Dashboard**:
- Real-time voltage, current, power charts
- Battery SOC percentage
- Charging phase indicator
- System temperature

## ⚙️ Configuring Battery & MPPT

### Default Settings (3S Li-Ion)
- **Charge Voltage**: 12.6V
- **Charge Current**: 1.0A
- **Battery Type**: Li-Ion
- **MPPT VOC**: 56.25% of open-circuit voltage (default)

### Change Configuration

**Via Web Interface** (easiest):
1. Go to **Settings** page
2. Select your battery type from dropdown
3. Adjust charge voltage/current limits
4. Enable/disable MPPT tracking
5. Click **Save** to persist to device

**Via Serial Debug**:
```cpp
// Available methods in firmware:
modbeeMPPT.printQuickStatus();        // Essential status only
modbeeMPPT.printStatus();             // Complete status
modbeeMPPT.printPowerMeasurements();  // Power readings
modbeeMPPT.printConfiguration();      // Current settings
modbeeMPPT.printFaults();            // Fault status
```

## 🔋 Battery Chemistry Selection

### Pre-configured Types
- **Li-Ion**: 4.2V/cell (common 1-4S packs)
- **LiFePO4**: 3.6V/cell (safer, longer life)
- **Li-Po**: 4.2V/cell (high performance)
- **Lead-Acid**: 2.0V/cell (traditional)

Select in Settings page → Battery Type dropdown.

**Adjustable Parameters**:
- Cell count (2, 3, or 4)
- Max charging voltage
- Charge/discharge current limits
- Device automatically calculates per-cell limits

## 📊 Understanding the Dashboard

### Main Status Indicators

| Indicator | Meaning |
|-----------|---------|
| **Input Voltage** | Solar panel or adapter voltage |
| **Input Current** | Power drawn from source |
| **Battery Voltage** | Current battery pack voltage |
| **Charge Current** | Current flowing into battery |
| **System Output** | Voltage available to loads on LOAD connector |
| **SOC %** | Estimated state of charge (0-100%) |

### Charging Phases

| Phase | Description |
|-------|-------------|
| **Pre-charge** | Low current (0.1-0.5A) until battery reaches safe threshold |
| **Bulk Charge** | Maximum charge current until ~80% SOC |
| **Absorption** | Constant voltage, decreasing current until full |
| **Float** | Trickle charge to maintain 100% SOC |
| **Idle** | No charging (battery full or no solar) |

## 🔌 Hardware Connections

### Connectors Used During Setup

| Connector | Use | Required |
|-----------|-----|----------|
| **J1** | Solar panel input | ✅ Primary power source |
| **J2** | Battery pack | ✅ Output/charge target |
| **J3** | Load output | Optional: power external devices |
| **USB-C** | Programming & debug | ✅ Power + serial terminal |

**Optional Connectors** (not needed for basic setup):
- J4 (RS485) - For networked systems
- J5 (Sol+/-) - Alternate solar input
- J6 (VCC-SYS) - External 12V output
- J7 (Grove I2C) - External sensors

## 🐛 Troubleshooting

### Web Interface Won't Load

**Problem**: Can't connect to http://192.168.4.1/

**Solutions**:
1. Verify WiFi connection to **ModbeeMPPT** network
2. Wait 5 seconds after power-up for web server to start
3. Check serial monitor for error messages
4. Re-upload filesystem using **Upload Filesystem Image**
5. Factory reset: Hold reset button 5+ seconds

### No Charging Occurring

**Problem**: Solar panel/battery connected but no charging

**Check**:
1. Is solar voltage >4V? (Minimum for BQ25798)
2. Is battery voltage in valid range? (>3V, <18.8V)
3. Open **Debug** page → Check charger status
4. Verify battery type matches actual pack (voltage mismatch disables charging)

### Ground/Polarity Issues

- **Reversed solar polarity**: Will not charge (protection active)
- **Reversed battery polarity**: Device won't detect battery
- **Fix**: Power off, check connections, power on

## 📚 Next Steps

1. **Experiment with Settings**: Try different charge currents, voltages
2. **Monitor Extended Charge**: Watch SOC percentage over time
3. **Test Load Output**: Connect device to J3 LOAD connector
4. **Review Logs**: Check Debug page for detailed measurements
5. **Advanced**: See [docs/SOFTWARE.md](./SOFTWARE.md) for firmware customization

## 📞 Support

**Not working?** Check:
1. [docs/HARDWARE.md](./HARDWARE.md) - Electrical specifications & safety
2. [docs/SOFTWARE.md](./SOFTWARE.md) - Firmware architecture & debugging
3. Serial monitor output at 115200 baud
4. Web dashboard Debug page for status codes
