/*!
 * @file ModbeeMpptWebServer.h
 * 
 * @brief Web server interface for ModbeeMPPT with WiFi management
 * 
 * This file handles WiFi AP mode, web server, WebSocket communication,
 * and power management for the ModbeeMPPT web interface.
 */

#ifndef MODBEE_MPPT_WEBSERVER_H
#define MODBEE_MPPT_WEBSERVER_H

#include "ModbeeMpptGlobal.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// WiFi Configuration
#define WIFI_SSID "ModbeeMPPT"
#define WIFI_PASSWORD ""  // Open network
#define WIFI_TIMEOUT_MS (5 * 60 * 1000)  // 5 minutes

// DNS Configuration
#define DNS_PORT 53

class ModbeeMpptWebServer {
public:
  ModbeeMpptWebServer(class ModbeeMPPT& mppt);
  
  // Server management
  bool begin();
  void loop();
  void stop();
  
  // WiFi management
   bool startWiFi(); // Keep WiFi management functions
   void stopWiFi();
   bool isClientConnected() const { return _clientConnected; }

   bool _wifiActive; // Track if WiFi is active
  
  // Power management
   // Removed power management functions
  
  // Data broadcasting
  void broadcastData();
  void broadcastSettings();
  void broadcastDebugData();
  
private:
  class ModbeeMPPT& _mppt;
  class ModbeeMpptLog* _statsLog;
  
  // Server components
  AsyncWebServer _server;
  AsyncWebSocket _webSocket;
  DNSServer _dnsServer;
  
  // State management
   bool _clientConnected; // Keep only necessary state variables
   unsigned long _lastActivity;
  
  // Button handling
   // Removed button handling variables
  
  // WebSocket clients
  void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                       AwsEventType type, void *arg, uint8_t *data, size_t len);
  
  // HTTP request handlers
  void handleRoot(AsyncWebServerRequest *request);
  void handleSettings(AsyncWebServerRequest *request);
  void handleDebug(AsyncWebServerRequest *request);
  void handleNotFound(AsyncWebServerRequest *request);
  
  // WebSocket command handlers
  void handleWebSocketMessage(AsyncWebSocketClient *client, const String& message);
  void sendSettings(AsyncWebSocketClient *client);
  void sendSystemData(AsyncWebSocketClient *client);
  void sendDebugData(AsyncWebSocketClient *client);
  void saveSettings(AsyncWebSocketClient *client, const JsonVariant& settings);
  void resetDefaults(AsyncWebSocketClient *client);
  
  // Utility functions
  String getSystemData();
  String getSettingsData();
  String getDebugData();
  String getRegisterData();
  String getFaultData();
  
  // Power management helpers
   // Removed power management helper functions
  void updateClientStatus();
};

#endif // MODBEE_MPPT_WEBSERVER_H
