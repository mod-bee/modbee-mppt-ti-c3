#ifndef MODBEE_MPPT_LOG_H
#define MODBEE_MPPT_LOG_H

#include <ArduinoJson.h>
#include <LittleFS.h>
#include "ModbeeMpptAPI.h"

#define MODBEE_STATS_FILE "/data/mppt_stats.json"

struct ModbeeMpptStats {
    float vin1TotalEnergyWh = 0.0f;
    float vin1PeakPower = 0.0f;
    float vin2TotalEnergyWh = 0.0f;
    float vin2PeakPower = 0.0f;
    float vbusTotalEnergyWh = 0.0f;
    float vbusPeakPower = 0.0f;
    float batteryTotalEnergyWh = 0.0f;
    float batteryPeakPower = 0.0f;
    float batteryPeakChargeAmps = 0.0f;
    float batteryPeakDischargeAmps = 0.0f;
    float batteryAmpHoursCharge = 0.0f;
    float batteryAmpHoursDischarge = 0.0f;
    float batteryPeakDischargePower = 0.0f;
    float batteryWattHoursDischarge = 0.0f;
    float systemTotalEnergyWh = 0.0f;
    float systemPeakPower = 0.0f;
};

class ModbeeMpptLog {
public:
    ModbeeMpptLog(ModbeeMpptAPI* api);
    bool begin();
    void loadStatsToAPI();
    void saveStatsFromAPI();
    void resetStatsAndAPI();
    bool resetStats();
    bool loadStats(ModbeeMpptStats& stats);
    bool saveStats(const ModbeeMpptStats& stats);
private:
    ModbeeMpptAPI* _api;
    unsigned long _lastSaveMs = 0;
    const unsigned long _saveIntervalMs = 300000; // 5 minutes
};

#endif // MODBEE_MPPT_LOG_H
