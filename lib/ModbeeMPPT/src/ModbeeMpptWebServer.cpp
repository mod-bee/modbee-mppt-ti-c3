/*!
 * @file ModbeeMpptWebServer.cpp
 * 
 * @brief Implementation of ModbeeMPPT web server
 */

#include "ModbeeMpptWebServer.h"
#include "ModbeeMPPT.h"
#include "ModbeeMpptLog.h"

ModbeeMpptWebServer::ModbeeMpptWebServer(ModbeeMPPT& mppt)
  : _mppt(mppt),
    _server(80),
    _webSocket("/ws"),
    _clientConnected(false),
    _lastActivity(0),
    _wifiActive(false) {
}

bool ModbeeMpptWebServer::begin() {
  // Initialize LittleFS for web files
  if (!LittleFS.begin()) {
    Serial.println("Failed to initialize LittleFS filesystem");
    return false;
  }
  Serial.println("LittleFS initialized successfully");
  
  // Setup WebSocket
  _webSocket.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                           AwsEventType type, void *arg, uint8_t *data, size_t len) {
    this->onWebSocketEvent(server, client, type, arg, data, len);
  });
  
  // Setup HTTP routes
  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleRoot(request);
  });
  
  _server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleSettings(request);
  });
  
  _server.on("/debug", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleDebug(request);
  });
  
  // Serve static files from LittleFS
  _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  
  // Add WebSocket to server
  _server.addHandler(&_webSocket);
  
  // Handle 404
  _server.onNotFound([this](AsyncWebServerRequest *request) {
    this->handleNotFound(request);
  });
  
  Serial.println("Web server initialized");
  return true;
}

void ModbeeMpptWebServer::loop() {
  
  // Only run webserver logic if enabled by ModbeeMPPT
  if (!_mppt.isWebServerEnabled()) {
    return;
  }
  
  // Auto-start WiFi when webserver is enabled (first time)
  static bool autoStarted = false;
  if (!autoStarted && !_wifiActive) {
    Serial.println("Auto-starting WiFi AP...");
    startWiFi();
    autoStarted = true;
  }
  
  if (_wifiActive) {
    updateClientStatus();
    
    // Broadcast data to connected clients every 1 seconds
    static unsigned long lastBroadcast = 0;
    if (millis() - lastBroadcast > 1000) {
      broadcastData();
      lastBroadcast = millis();
    }
  }
}

void ModbeeMpptWebServer::stop() {
  stopWiFi();
}

bool ModbeeMpptWebServer::startWiFi() {
  if (_wifiActive) return true;
  
  Serial.println("Starting WiFi AP...");
  
  // Start WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("WiFi AP started. IP: ");
  Serial.println(WiFi.softAPIP());
  
  // Start DNS server for captive portal
  _dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  
  // Start web server
  _server.begin();
  
  _wifiActive = true;
  _lastActivity = millis();
  
  Serial.println("Web server started on http://192.168.4.1");
  return true;
}

void ModbeeMpptWebServer::stopWiFi() {
  if (!_wifiActive) return;
  
  Serial.println("Stopping WiFi and web server...");
  
  _server.end();
  _dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  
  _wifiActive = false;
  _clientConnected = false;
  
  Serial.println("WiFi stopped");
}

void ModbeeMpptWebServer::updateClientStatus() {
  _clientConnected = (_webSocket.count() > 0);
}

void ModbeeMpptWebServer::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                                          AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT: {
      // Check if we already have too many connections from the same IP
      String clientIP = client->remoteIP().toString();
      int connectionsFromIP = 0;
      
      // Count connections from this IP
      for (size_t i = 0; i < server->count(); i++) {
        AsyncWebSocketClient* c = server->client(i);
        if (c && c->remoteIP().toString() == clientIP) {
          connectionsFromIP++;
        }
      }
      
      // If more than 2 connections from same IP, close the oldest one
      if (connectionsFromIP > 2) {
        for (size_t i = 0; i < server->count(); i++) {
          AsyncWebSocketClient* c = server->client(i);
          if (c && c->remoteIP().toString() == clientIP && c->id() != client->id()) {
            Serial.printf("Closing old WebSocket connection #%u from %s (too many connections)\n", 
                         c->id(), clientIP.c_str());
            c->close();
            break;
          }
        }
      }
      
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), clientIP.c_str());
      _clientConnected = true;
      
      // Send initial data to new client
      sendSystemData(client);
      sendSettings(client);
      sendDebugData(client);
      break;
    }
      
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      updateClientStatus();
      break;
      
    case WS_EVT_DATA: {
      AwsFrameInfo *info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String message;
        for (size_t i = 0; i < len; i++) {
          message += (char)data[i];
        }
        handleWebSocketMessage(client, message);
      }
      break;
    }
      
    case WS_EVT_ERROR:
      Serial.printf("WebSocket client #%u error(%u): %s\n", client->id(), *((uint16_t*)arg), (char*)data);
      break;
  }
}

void ModbeeMpptWebServer::handleWebSocketMessage(AsyncWebSocketClient *client, const String& message) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.println("Failed to parse WebSocket message");
    return;
  }
  
  String command = doc["command"].as<String>();
  
  if (command == "getSettings") {
    sendSettings(client);
  } else if (command == "getDebugData") {
    sendDebugData(client);
  } else if (command == "getSystemData") {
    sendSystemData(client);
  } else if (command == "saveSettings") {
    if (doc["settings"].is<JsonObject>()) {
      saveSettings(client, doc["settings"]);
    }
  } else if (command == "resetDefaults" || command == "resetSettings") {
    resetDefaults(client);
  } else if (command == "resetStat") {
    String domain = doc["domain"].as<String>();
    if (domain == "vin1") {
      _mppt.api.resetVin1Stats();
    } else if (domain == "vin2") {
      _mppt.api.resetVin2Stats();
    } else if (domain == "vbus") {
      _mppt.api.resetVbusStats();
    } else if (domain == "vbat") {
      _mppt.api.resetBatteryStats();
    } else if (domain == "vsys") {
      _mppt.api.resetSystemStats();
    }
    sendSystemData(client); // Send updated stats after reset
  } else if (command == "resetBatteryAmpStats") {
    _mppt.api.resetBatteryAmpStats();
    sendSystemData(client);
  } else if (command == "resetBatteryDischargePowerStats") {
    _mppt.api.resetBatteryDischargePowerStats();
    sendSystemData(client);
  }
}

void ModbeeMpptWebServer::sendSettings(AsyncWebSocketClient *client) {
  String response = getSettingsData();
  if (client) {
    client->text(response);
  } else {
    _webSocket.textAll(response);
  }
}

void ModbeeMpptWebServer::sendSystemData(AsyncWebSocketClient *client) {
  String response = getSystemData();
  if (client) {
    client->text(response);
  } else {
    _webSocket.textAll(response);
  }
}

void ModbeeMpptWebServer::sendDebugData(AsyncWebSocketClient *client) {
  String response = getDebugData();
  if (client) {
    client->text(response);
  } else {
    _webSocket.textAll(response);
  }
}

void ModbeeMpptWebServer::saveSettings(AsyncWebSocketClient *client, const JsonVariant& settings) {
  JsonDocument response;
  response["type"] = "status";
  
  Serial.println("=== SAVING SETTINGS DEBUG ===");
  Serial.println("Received settings data:");
  String settingsStr;
  serializeJsonPretty(settings, settingsStr);
  Serial.println(settingsStr);
  
  // Simple direct updates to config data
  bool success = true;
  
  try {
    // Battery configuration - Using ArduinoJson v7 recommended approach
    // Read values directly - use proper ArduinoJson v7 syntax
    if (auto val = settings["batteryType"]; !val.isNull()) {
      int batteryTypeValue = val.as<int>();
      Serial.printf("Setting battery type: %d -> %d\n", _mppt.config.data.battery_type, batteryTypeValue);
      _mppt.config.data.battery_type = (modbee_battery_type_t)batteryTypeValue;
    }
    if (auto val = settings["cellCount"]; !val.isNull()) {
      Serial.printf("Setting cell count: %d -> %d\n", _mppt.config.data.battery_cell_count, val.as<uint8_t>());
      _mppt.config.data.battery_cell_count = val.as<uint8_t>();
    }
    if (auto val = settings["chargeVoltage"]; !val.isNull()) {
      Serial.printf("Setting charge voltage: %.2f -> %.2f\n", _mppt.config.data.charge_voltage, val.as<float>());
      _mppt.config.data.charge_voltage = val.as<float>();
    }
    if (auto val = settings["chargeCurrent"]; !val.isNull()) {
      Serial.printf("Setting charge current: %.2f -> %.2f\n", _mppt.config.data.charge_current, val.as<float>());
      _mppt.config.data.charge_current = val.as<float>();
    }
    if (auto val = settings["termCurrent"]; !val.isNull()) {
      Serial.printf("Setting termination current: %.3f -> %.3f\n", _mppt.config.data.termination_current, val.as<float>());
      _mppt.config.data.termination_current = val.as<float>();
    }
    if (auto val = settings["prechargeCurrent"]; !val.isNull()) {
      Serial.printf("Setting precharge current: %.3f -> %.3f\n", _mppt.config.data.precharge_current, val.as<float>());
      _mppt.config.data.precharge_current = val.as<float>();
    }
    if (auto val = settings["prechargeVoltageThreshold"]; !val.isNull()) {
      int prechargeVoltageThresholdValue = val.as<int>();
      Serial.printf("Setting precharge voltage threshold: %d -> %d\n", _mppt.config.data.precharge_voltage_threshold, prechargeVoltageThresholdValue);
      _mppt.config.data.precharge_voltage_threshold = (modbee_vbat_lowv_t)prechargeVoltageThresholdValue;
    }
    if (auto val = settings["rechargeThreshold"]; !val.isNull()) {
      Serial.printf("Setting recharge threshold: %.3f -> %.3f\n", _mppt.config.data.recharge_threshold, val.as<float>());
      _mppt.config.data.recharge_threshold = val.as<float>();
    }
    if (auto val = settings["inputVoltage"]; !val.isNull()) {
      Serial.printf("Setting input voltage limit: %.1f -> %.1f\n", _mppt.config.data.input_voltage_limit, val.as<float>());
      _mppt.config.data.input_voltage_limit = val.as<float>();
    }
    if (auto val = settings["inputCurrent"]; !val.isNull()) {
      Serial.printf("Setting input current limit: %.2f -> %.2f\n", _mppt.config.data.input_current_limit, val.as<float>());
      _mppt.config.data.input_current_limit = val.as<float>();
    }
    if (auto val = settings["vacOvp"]; !val.isNull()) {
      Serial.printf("Setting VAC OVP threshold: %.1f -> %.1f\n", _mppt.config.data.vac_ovp_threshold, val.as<float>());
      _mppt.config.data.vac_ovp_threshold = val.as<float>();
    }
    if (auto val = settings["mpptEnable"]; !val.isNull()) {
      Serial.printf("Setting MPPT enable: %s -> %s\n", _mppt.config.data.mppt_enable ? "true" : "false", val.as<bool>() ? "true" : "false");
      _mppt.config.data.mppt_enable = val.as<bool>();
    }
    if (auto val = settings["systemVoltage"]; !val.isNull()) {
      Serial.printf("Setting min system voltage: %.1f -> %.1f\n", _mppt.config.data.min_system_voltage, val.as<float>());
      _mppt.config.data.min_system_voltage = val.as<float>();
    }
    
    // MPPT timer and advanced settings
    if (auto val = settings["vocPercent"]; !val.isNull()) {
      int vocPercentValue = val.as<int>();
      Serial.printf("Setting VOC percent: %d -> %d\n", _mppt.config.data.mppt_voc_percent, vocPercentValue);
      _mppt.config.data.mppt_voc_percent = (modbee_voc_percent_t)vocPercentValue;
    }
    if (auto val = settings["vocDelay"]; !val.isNull()) {
      int vocDelayValue = val.as<int>();
      Serial.printf("Setting VOC delay: %d -> %d\n", _mppt.config.data.mppt_voc_delay, vocDelayValue);
      _mppt.config.data.mppt_voc_delay = (modbee_voc_delay_t)vocDelayValue;
    }
    if (auto val = settings["vocRate"]; !val.isNull()) {
      int vocRateValue = val.as<int>();
      Serial.printf("Setting VOC rate: %d -> %d\n", _mppt.config.data.mppt_voc_rate, vocRateValue);
      _mppt.config.data.mppt_voc_rate = (modbee_voc_rate_t)vocRateValue;
    }
    if (auto val = settings["chargeTimer"]; !val.isNull()) {
      int chargeTimerValue = val.as<int>();
      Serial.printf("Setting charge timer: %d -> %d\n", _mppt.config.data.fast_charge_timer, chargeTimerValue);
      _mppt.config.data.fast_charge_timer = (modbee_charge_timer_t)chargeTimerValue;
    }
    if (auto val = settings["chargeTimerEnable"]; !val.isNull()) {
      Serial.printf("Setting charge timer enable: %s -> %s\n", _mppt.config.data.fast_charge_timer_enable ? "true" : "false", val.as<bool>() ? "true" : "false");
      _mppt.config.data.fast_charge_timer_enable = val.as<bool>();
    }
    if (auto val = settings["prechargeTimer"]; !val.isNull()) {
      int prechargeTimerValue = val.as<int>();
      Serial.printf("Setting precharge timer: %d -> %d\n", _mppt.config.data.precharge_timer, prechargeTimerValue);
      _mppt.config.data.precharge_timer = (modbee_precharge_timer_t)prechargeTimerValue;
    }
    if (auto val = settings["prechargeTimerEnable"]; !val.isNull()) {
      Serial.printf("Setting precharge timer enable: %s -> %s\n", _mppt.config.data.precharge_timer_enable ? "true" : "false", val.as<bool>() ? "true" : "false");
      _mppt.config.data.precharge_timer_enable = val.as<bool>();
    }
    if (auto val = settings["topoffTimer"]; !val.isNull()) {
      int topoffTimerValue = val.as<int>();
      Serial.printf("Setting topoff timer: %d -> %d\n", _mppt.config.data.topoff_timer, topoffTimerValue);
      _mppt.config.data.topoff_timer = (modbee_topoff_timer_t)topoffTimerValue;
    }
    
    // Power Management & Noise Control settings
    if (auto val = settings["pfmForwardEnable"]; !val.isNull()) {
      Serial.printf("Setting forward PFM enable: %s -> %s\n", _mppt.config.data.pfm_forward_enable ? "true" : "false", val.as<bool>() ? "true" : "false");
      _mppt.config.data.pfm_forward_enable = val.as<bool>();
    }
    if (auto val = settings["ooaForwardEnable"]; !val.isNull()) {
      Serial.printf("Setting forward OOA enable: %s -> %s\n", _mppt.config.data.ooa_forward_enable ? "true" : "false", val.as<bool>() ? "true" : "false");
      _mppt.config.data.ooa_forward_enable = val.as<bool>();
    }
    
    // System Intervals
    if (auto val = settings["batteryCheckInterval"]; !val.isNull()) {
      unsigned long newInterval = val.as<unsigned long>();
      Serial.printf("Setting battery check interval: %lu -> %lu\n", _mppt.config.data.battery_check_interval, newInterval);
      _mppt.config.data.battery_check_interval = newInterval;
    }
    if (auto val = settings["socCheckInterval"]; !val.isNull()) {
      unsigned long newInterval = val.as<unsigned long>();
      Serial.printf("Setting SOC check interval: %lu -> %lu\n", _mppt.config.data.soc_check_interval, newInterval);
      _mppt.config.data.soc_check_interval = newInterval;
    }
    
    Serial.println("=== VALIDATING CONFIG ===");
    if (!_mppt.config.validateConfig()) {
      Serial.println("ERROR: Config validation failed!");
      success = false;
    } else {
      Serial.println("Config validation passed");
    }
    
    // Save to file
    if (success) {
      Serial.println("=== SAVING CONFIG TO FILE ===");
      success = _mppt.config.saveConfig();
      if (success) {
        Serial.println("Config saved to file successfully");
      } else {
        Serial.println("ERROR: Failed to save config to file");
      }
    }
    
    // Apply to hardware
    if (success) {
      Serial.println("=== APPLYING CONFIG TO MPPT ===");
      bool applySuccess = _mppt.config.applyToMPPT(_mppt.api);
      if (applySuccess) {
        Serial.println("Config applied to MPPT successfully");
      } else {
        Serial.println("ERROR: Failed to apply config to MPPT");
        success = false;
      }
    }
    
  } catch (...) {
    Serial.println("ERROR: Exception caught during settings save");
    success = false;
  }
  
  Serial.printf("=== SAVE RESULT: %s ===\n", success ? "SUCCESS" : "FAILED");
  
  response["success"] = success;
  response["message"] = success ? "Settings saved successfully" : "Failed to save settings";
  
  String responseStr;
  serializeJson(response, responseStr);
  client->text(responseStr);
  
  if (success) {
    broadcastSettings();
  }
}

void ModbeeMpptWebServer::resetDefaults(AsyncWebSocketClient *client) {
  bool success = _mppt.resetConfig();
  
  JsonDocument response;
  response["type"] = "status";
  response["success"] = success;
  response["message"] = success ? "Settings reset to defaults" : "Failed to reset settings";
  
  String responseStr;
  serializeJson(response, responseStr);
  client->text(responseStr);
  
  if (success) {
    broadcastSettings();
  }
}

void ModbeeMpptWebServer::broadcastData() {
  if (_webSocket.count() > 0) {
    String data = getSystemData();
    Serial.printf("Broadcasting data to %d clients: %s\n", _webSocket.count(), data.c_str());
    _webSocket.textAll(data);
  }
}

void ModbeeMpptWebServer::broadcastSettings() {
  if (_webSocket.count() > 0) {
    String data = getSettingsData();
    _webSocket.textAll(data);
  }
}

void ModbeeMpptWebServer::broadcastDebugData() {
  if (_webSocket.count() > 0) {
    String data = getDebugData();
    _webSocket.textAll(data);
  }
}

String ModbeeMpptWebServer::getSystemData() {
  JsonDocument doc;
  doc["type"] = "data";

  // Load persistent stats from JSON file
  ModbeeMpptStats stats;
  bool loaded = false;
  if (_statsLog) {
    loaded = _statsLog->loadStats(stats);
  }

  // VIN1/VAC1
  doc["vin1PeakPower"] = loaded ? stats.vin1PeakPower : _mppt.api.getVin1PeakPower();
  doc["vin1TotalEnergyWh"] = loaded ? stats.vin1TotalEnergyWh : _mppt.api.getVin1TotalEnergyWh();
  // VIN2/VAC2
  doc["vin2PeakPower"] = loaded ? stats.vin2PeakPower : _mppt.api.getVin2PeakPower();
  doc["vin2TotalEnergyWh"] = loaded ? stats.vin2TotalEnergyWh : _mppt.api.getVin2TotalEnergyWh();
  // VBUS
  doc["vbusPeakPower"] = loaded ? stats.vbusPeakPower : _mppt.api.getVbusPeakPower();
  doc["vbusTotalEnergyWh"] = loaded ? stats.vbusTotalEnergyWh : _mppt.api.getVbusTotalEnergyWh();
  // Battery
  doc["batteryPeakPower"] = loaded ? stats.batteryPeakPower : _mppt.api.getBatteryPeakPower();
  doc["batteryTotalEnergyWh"] = loaded ? stats.batteryTotalEnergyWh : _mppt.api.getBatteryTotalEnergyWh();
  doc["batteryPeakChargeAmps"] = loaded ? stats.batteryPeakChargeAmps : _mppt.api.getBatteryPeakChargeAmps();
  doc["batteryPeakDischargeAmps"] = loaded ? stats.batteryPeakDischargeAmps : _mppt.api.getBatteryPeakDischargeAmps();
  doc["batteryAmpHoursCharge"] = loaded ? stats.batteryAmpHoursCharge : _mppt.api.getBatteryAmpHoursCharge();
  doc["batteryAmpHoursDischarge"] = loaded ? stats.batteryAmpHoursDischarge : _mppt.api.getBatteryAmpHoursDischarge();
  doc["batteryPeakDischargePower"] = loaded ? stats.batteryPeakDischargePower : _mppt.api.getBatteryPeakDischargePower();
  doc["batteryWattHoursDischarge"] = loaded ? stats.batteryWattHoursDischarge : _mppt.api.getBatteryWattHoursDischarge();
  // System
  doc["systemPeakPower"] = loaded ? stats.systemPeakPower : _mppt.api.getSystemPeakPower();
  doc["systemTotalEnergyWh"] = loaded ? stats.systemTotalEnergyWh : _mppt.api.getSystemTotalEnergyWh();

  // Live measurements (not persistent)
  modbee_power_data_t vac1Power = _mppt.api.getVAC1Power();
  doc["vac1Voltage"] = vac1Power.voltage;
  doc["vac1Current"] = vac1Power.current;
  doc["vac1Power"] = vac1Power.power;
  modbee_power_data_t vac2Power = _mppt.api.getVAC2Power();
  doc["vac2Voltage"] = vac2Power.voltage;
  doc["vac2Current"] = vac2Power.current;
  doc["vac2Power"] = vac2Power.power;
  modbee_power_data_t vbusPower = _mppt.api.getVbusPower();
  doc["vbusVoltage"] = vbusPower.voltage;
  doc["vbusCurrent"] = vbusPower.current;
  doc["vbusPower"] = vbusPower.power;
  modbee_power_data_t systemPower = _mppt.api.getSystemPower();
  doc["vsysVoltage"] = systemPower.voltage;
  doc["vsysCurrent"] = systemPower.current;
  doc["vsysPower"] = systemPower.power;
  modbee_power_data_t batteryPower = _mppt.api.getBatteryPower();
  doc["vbatVoltage"] = batteryPower.voltage;
  doc["vbatCurrent"] = batteryPower.current;
  doc["vbatPower"] = batteryPower.power;
  doc["vbatTrueVoltage"] = _mppt.api.getTrueBatteryVoltage();

  // Battery SOC
  doc["actualSOC"] = _mppt.api.getActualBatterySOC();
  doc["usableSOC"] = _mppt.api.getUsableBatterySOC();
  doc["chargePercent"] = _mppt.api.getBatteryChargePercent();

  // System status
  doc["isCharging"] = _mppt.api.isCharging();
  doc["hasFaults"] = _mppt.api.hasFaults();
  doc["chargeState"] = _mppt.api.getChargeStateString();
  doc["mpptEnabled"] = _mppt.api.getMPPTEnable();
  doc["batteryConnected"] = _mppt.api.detectBatteryConnected();

  // Temperature
  doc["dieTemperature"] = _mppt.api.getDieTemperature();
  doc["batteryTemperature"] = _mppt.api.getBatteryTemperature();

  String result;
  serializeJson(doc, result);
  return result;
}

String ModbeeMpptWebServer::getSettingsData() {
  JsonDocument doc;
  doc["type"] = "settings";
  
  JsonObject settings = doc["settings"].to<JsonObject>();
  
  // Get actual settings from config data - direct access!
  settings["batteryType"] = (int)_mppt.config.data.battery_type;
  settings["cellCount"] = _mppt.config.data.battery_cell_count;
  settings["chargeVoltage"] = _mppt.config.data.charge_voltage;
  settings["chargeCurrent"] = _mppt.config.data.charge_current;
  settings["termCurrent"] = _mppt.config.data.termination_current;
  settings["prechargeCurrent"] = _mppt.config.data.precharge_current;
  settings["prechargeVoltageThreshold"] = (int)_mppt.config.data.precharge_voltage_threshold;
  settings["rechargeThreshold"] = _mppt.config.data.recharge_threshold;
  settings["inputVoltage"] = _mppt.config.data.input_voltage_limit;
  settings["inputCurrent"] = _mppt.config.data.input_current_limit;
  settings["vacOvp"] = _mppt.config.data.vac_ovp_threshold;
  settings["mpptEnable"] = _mppt.config.data.mppt_enable;
  settings["vocPercent"] = (int)_mppt.config.data.mppt_voc_percent;
  settings["vocDelay"] = (int)_mppt.config.data.mppt_voc_delay;
  settings["vocRate"] = (int)_mppt.config.data.mppt_voc_rate;
  settings["chargeTimer"] = (int)_mppt.config.data.fast_charge_timer;
  settings["chargeTimerEnable"] = _mppt.config.data.fast_charge_timer_enable;
  settings["prechargeTimer"] = (int)_mppt.config.data.precharge_timer;
  settings["prechargeTimerEnable"] = _mppt.config.data.precharge_timer_enable;
  settings["topoffTimer"] = (int)_mppt.config.data.topoff_timer;
  settings["systemVoltage"] = _mppt.config.data.min_system_voltage;
  settings["pfmForwardEnable"] = _mppt.config.data.pfm_forward_enable;
  settings["ooaForwardEnable"] = _mppt.config.data.ooa_forward_enable;
  settings["batteryCheckInterval"] = _mppt.config.data.battery_check_interval;
  settings["socCheckInterval"] = _mppt.config.data.soc_check_interval;
  
  String result;
  serializeJson(doc, result);
  return result;
}

String ModbeeMpptWebServer::getDebugData() {
  JsonDocument doc;
  doc["type"] = "debug";
  
  // All measurements with high precision using proper power data structures
  modbee_power_data_t vbusPower = _mppt.api.getVbusPower();
  doc["vbusVoltage"] = String(vbusPower.voltage, 3);
  doc["ibusCurrent"] = String(vbusPower.current, 3);
  doc["vbusPower"] = String(vbusPower.power, 3);
  
  modbee_power_data_t batteryPower = _mppt.api.getBatteryPower();
  doc["vbatVoltage"] = String(batteryPower.voltage, 3);
  doc["batteryCurrent"] = String(batteryPower.current, 3);
  doc["batteryPower"] = String(batteryPower.power, 3);
  
  modbee_power_data_t systemPower = _mppt.api.getSystemPower();
  doc["vsysVoltage"] = String(systemPower.voltage, 3);
  doc["systemCurrent"] = String(systemPower.current, 3);
  doc["systemPower"] = String(systemPower.power, 3);
  
  modbee_power_data_t vac1Power = _mppt.api.getVAC1Power();
  doc["vac1Voltage"] = String(vac1Power.voltage, 3);
  doc["vac1Current"] = String(vac1Power.current, 3);
  doc["vac1Power"] = String(vac1Power.power, 3);
  
  modbee_power_data_t vac2Power = _mppt.api.getVAC2Power();
  doc["vac2Voltage"] = String(vac2Power.voltage, 3);
  doc["vac2Current"] = String(vac2Power.current, 3);
  doc["vac2Power"] = String(vac2Power.power, 3);
  
  doc["trueBatteryVoltage"] = String(_mppt.api.getTrueBatteryVoltage(), 3);
  doc["temperature"] = String(_mppt.api.getDieTemperature(), 1);
  
  // Battery SOC using proper API methods
  doc["actualSOC"] = String(_mppt.api.getActualBatterySOC(), 1);
  doc["usableSOC"] = String(_mppt.api.getUsableBatterySOC(), 1);
  doc["chargePercent"] = String(_mppt.api.getBatteryChargePercent(), 1);
  
  // System status strings from API
  doc["chargeState"] = _mppt.api.getChargeStateString();
  doc["faultStatus"] = _mppt.api.getFaultString();
  doc["mpptEnabled"] = _mppt.api.getMPPTEnable();
  doc["batteryConnected"] = _mppt.api.detectBatteryConnected();
  
  // Status register sections with decoded strings from API
  JsonObject statusRegs = doc["statusRegisters"].to<JsonObject>();
  statusRegs["status0"] = _mppt.api.getStatus0String();
  statusRegs["status1"] = _mppt.api.getStatus1String();
  statusRegs["status2"] = _mppt.api.getStatus2String();
  statusRegs["status3"] = _mppt.api.getStatus3String();
  statusRegs["status4"] = _mppt.api.getStatus4String();
  
  // Configuration values - ALL settings from API
  JsonObject config = doc["configuration"].to<JsonObject>();
  
  // Basic charging configuration
  config["chargeVoltage"] = String(_mppt.api.getChargeVoltage(), 3);
  config["chargeCurrent"] = String(_mppt.api.getChargeCurrent(), 3);
  config["terminationCurrent"] = String(_mppt.api.getTerminationCurrent(), 3);
  config["rechargeThreshold"] = String(_mppt.api.getRechargeThreshold(), 3);
  config["prechargeCurrent"] = String(_mppt.api.getPrechargeCurrent(), 3);
  config["prechargeVoltageThreshold"] = (int)_mppt.api.getPrechargeVoltageThreshold();
  
  // Input limits and protection
  config["inputCurrentLimit"] = String(_mppt.api.getInputCurrentLimit(), 3);
  config["inputVoltageLimit"] = String(_mppt.api.getInputVoltageLimit(), 3);
  config["minSystemVoltage"] = String(_mppt.api.getMinSystemVoltage(), 3);
  config["vacOVPThreshold"] = String(_mppt.api.getVACOVP(), 1);
  
  // System configuration
  config["cellCount"] = _mppt.api.getCellCount();
  config["chargeEnable"] = _mppt.api.getChargeEnable();
  config["hizMode"] = _mppt.api.getHIZMode();
  config["backupMode"] = _mppt.api.getBackupMode();
  config["shipMode"] = _mppt.api.getShipMode();
  config["batteryDischargeSense"] = _mppt.api.getBatteryDischargeSenseEnable();
  
  // Timer configuration
  config["fastChargeTimerEnable"] = _mppt.api.getFastChargeTimerEnable();
  config["prechargeTimerEnable"] = _mppt.api.getPrechargeTimerEnable();
  config["topoffTimer"] = (int)_mppt.api.getTopOffTimer();
  config["trickleChargeTimerEnable"] = _mppt.api.getTrickleChargeTimerEnable();
  config["timerHalfRateEnable"] = _mppt.api.getTimerHalfRateEnable();
  
  // Watchdog configuration
  config["watchdogEnable"] = _mppt.api.getWatchdogEnable();
  config["watchdogTimer"] = (int)_mppt.api.getWatchdogTimer();
  
  // MPPT configuration
  config["mpptEnable"] = _mppt.api.getMPPTEnable();
  config["mpptVOCPercent"] = (int)_mppt.api.getMPPTVOCPercent();
  config["mpptVOCPercentFloat"] = String(_mppt.api.vocPercentToFloat(_mppt.api.getMPPTVOCPercent()), 2);
  
  String result;
  serializeJson(doc, result);
  return result;
}

void ModbeeMpptWebServer::handleRoot(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", "text/html");
}

void ModbeeMpptWebServer::handleSettings(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/settings.html", "text/html");
}

void ModbeeMpptWebServer::handleDebug(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/debug.html", "text/html");
}

void ModbeeMpptWebServer::handleNotFound(AsyncWebServerRequest *request) {
  // Captive portal - redirect to main page
  request->redirect("/");
}
