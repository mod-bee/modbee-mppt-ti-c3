#include "ModbeeMpptLog.h"
#include "ModbeeMPPT.h"

ModbeeMpptLog::ModbeeMpptLog(ModbeeMpptAPI* api) : _api(api) {}

bool ModbeeMpptLog::begin() {
    if (!LittleFS.begin()) return false;
    if (!LittleFS.exists("/data")) LittleFS.mkdir("/data");
    // Create file if missing (initialize with current API values)
    if (!LittleFS.exists(MODBEE_STATS_FILE)) {
        saveStatsFromAPI(); // Create and initialize stats file
    }
    return true;
}

void ModbeeMpptLog::loadStatsToAPI() {
    ModbeeMpptStats stats;
    if (loadStats(stats) && _api) {
        // Restore loaded stats into API
        _api->setVin1TotalEnergyWh(stats.vin1TotalEnergyWh);
        _api->setVin1PeakPower(stats.vin1PeakPower);
        _api->setVin2TotalEnergyWh(stats.vin2TotalEnergyWh);
        _api->setVin2PeakPower(stats.vin2PeakPower);
        _api->setVbusTotalEnergyWh(stats.vbusTotalEnergyWh);
        _api->setVbusPeakPower(stats.vbusPeakPower);
        _api->setBatteryTotalEnergyWh(stats.batteryTotalEnergyWh);
        _api->setBatteryPeakPower(stats.batteryPeakPower);
        _api->setBatteryAmpHoursCharge(stats.batteryAmpHoursCharge);
        _api->setBatteryAmpHoursDischarge(stats.batteryAmpHoursDischarge);
        _api->setBatteryPeakChargeAmps(stats.batteryPeakChargeAmps);
        _api->setBatteryPeakDischargeAmps(stats.batteryPeakDischargeAmps);
        _api->setBatteryWattHoursDischarge(stats.batteryWattHoursDischarge);
        _api->setBatteryPeakDischargePower(stats.batteryPeakDischargePower);
        _api->setSystemTotalEnergyWh(stats.systemTotalEnergyWh);
        _api->setSystemPeakPower(stats.systemPeakPower);
    }
}

void ModbeeMpptLog::saveStatsFromAPI() {
    if (!_api) return;
    ModbeeMpptStats stats;
    stats.vin1TotalEnergyWh = _api->getVin1TotalEnergyWh();
    stats.vin1PeakPower = _api->getVin1PeakPower();
    stats.vin2TotalEnergyWh = _api->getVin2TotalEnergyWh();
    stats.vin2PeakPower = _api->getVin2PeakPower();
    stats.vbusTotalEnergyWh = _api->getVbusTotalEnergyWh();
    stats.vbusPeakPower = _api->getVbusPeakPower();
    stats.batteryTotalEnergyWh = _api->getBatteryTotalEnergyWh();
    stats.batteryPeakPower = _api->getBatteryPeakPower();
    stats.batteryPeakChargeAmps = _api->getBatteryPeakChargeAmps();
    stats.batteryPeakDischargeAmps = _api->getBatteryPeakDischargeAmps();
    stats.batteryAmpHoursCharge = _api->getBatteryAmpHoursCharge();
    stats.batteryAmpHoursDischarge = _api->getBatteryAmpHoursDischarge();
    stats.batteryPeakDischargePower = _api->getBatteryPeakDischargePower();
    stats.batteryWattHoursDischarge = _api->getBatteryWattHoursDischarge();
    stats.systemTotalEnergyWh = _api->getSystemTotalEnergyWh();
    stats.systemPeakPower = _api->getSystemPeakPower();
    saveStats(stats);
}

void ModbeeMpptLog::resetStatsAndAPI() {
    if (_api) {
        _api->resetVin1Stats();
        _api->resetVin2Stats();
        _api->resetVbusStats();
        _api->resetBatteryStats();
        _api->resetBatteryAmpStats();
        _api->resetBatteryDischargePowerStats();
        _api->resetSystemStats();
    }
    resetStats();
}

bool ModbeeMpptLog::loadStats(ModbeeMpptStats& stats) {
    if (!LittleFS.exists(MODBEE_STATS_FILE)) return false;
    File file = LittleFS.open(MODBEE_STATS_FILE, "r");
    if (!file) return false;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) return false;
    stats.vin1TotalEnergyWh = doc["vin1TotalEnergyWh"] | 0.0f;
    stats.vin1PeakPower = doc["vin1PeakPower"] | 0.0f;
    stats.vin2TotalEnergyWh = doc["vin2TotalEnergyWh"] | 0.0f;
    stats.vin2PeakPower = doc["vin2PeakPower"] | 0.0f;
    stats.vbusTotalEnergyWh = doc["vbusTotalEnergyWh"] | 0.0f;
    stats.vbusPeakPower = doc["vbusPeakPower"] | 0.0f;
    stats.batteryTotalEnergyWh = doc["batteryTotalEnergyWh"] | 0.0f;
    stats.batteryPeakPower = doc["batteryPeakPower"] | 0.0f;
    stats.batteryPeakChargeAmps = doc["batteryPeakChargeAmps"] | 0.0f;
    stats.batteryPeakDischargeAmps = doc["batteryPeakDischargeAmps"] | 0.0f;
    stats.batteryAmpHoursCharge = doc["batteryAmpHoursCharge"] | 0.0f;
    stats.batteryAmpHoursDischarge = doc["batteryAmpHoursDischarge"] | 0.0f;
    stats.batteryPeakDischargePower = doc["batteryPeakDischargePower"] | 0.0f;
    stats.batteryWattHoursDischarge = doc["batteryWattHoursDischarge"] | 0.0f;
    stats.systemTotalEnergyWh = doc["systemTotalEnergyWh"] | 0.0f;
    stats.systemPeakPower = doc["systemPeakPower"] | 0.0f;
    return true;
}

bool ModbeeMpptLog::saveStats(const ModbeeMpptStats& stats) {
    JsonDocument doc;
    doc["vin1TotalEnergyWh"] = stats.vin1TotalEnergyWh;
    doc["vin1PeakPower"] = stats.vin1PeakPower;
    doc["vin2TotalEnergyWh"] = stats.vin2TotalEnergyWh;
    doc["vin2PeakPower"] = stats.vin2PeakPower;
    doc["vbusTotalEnergyWh"] = stats.vbusTotalEnergyWh;
    doc["vbusPeakPower"] = stats.vbusPeakPower;
    doc["batteryTotalEnergyWh"] = stats.batteryTotalEnergyWh;
    doc["batteryPeakPower"] = stats.batteryPeakPower;
    doc["batteryPeakChargeAmps"] = stats.batteryPeakChargeAmps;
    doc["batteryPeakDischargeAmps"] = stats.batteryPeakDischargeAmps;
    doc["batteryAmpHoursCharge"] = stats.batteryAmpHoursCharge;
    doc["batteryAmpHoursDischarge"] = stats.batteryAmpHoursDischarge;
    doc["batteryPeakDischargePower"] = stats.batteryPeakDischargePower;
    doc["batteryWattHoursDischarge"] = stats.batteryWattHoursDischarge;
    doc["systemTotalEnergyWh"] = stats.systemTotalEnergyWh;
    doc["systemPeakPower"] = stats.systemPeakPower;
    File file = LittleFS.open(MODBEE_STATS_FILE, "w");
    if (!file) return false;
    size_t written = serializeJsonPretty(doc, file);
    file.close();
    return written > 0;
}

bool ModbeeMpptLog::resetStats() {
    if (LittleFS.exists(MODBEE_STATS_FILE)) LittleFS.remove(MODBEE_STATS_FILE);
    return true;
}
