#include "ModbeeMpptGlobal.h"
#include "ModbeeMPPT.h"

ModbeeMPPT modbeeMPPT;

unsigned long lastReadTime = 0;
const long readInterval = 2000;

void setup() {
  Serial.begin(115200);
  delay(500); // Allow time for the serial monitor to open

  Serial.println("=== ModbeeMPPT API Example ===");

  // Initialize LEDs
  modbeeMPPT.initializeLEDs();

  // Initialize ModbeeMPPT (handles I2C, BQ25798, and configuration)
  modbeeMPPT.begin();

  Serial.println("MPPT controller initialized and configured successfully!");
  
  // Initialize and enable web server (WiFi AP, DNS, WebSocket)
  modbeeMPPT.initWebServer();
  Serial.println("Web server should start automatically...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Call ModbeeMPPT loop for periodic battery detection, management, LED updates, and web server
  modbeeMPPT.loop();
  
  // Print status every 2 seconds
  if (currentTime - lastReadTime >= readInterval) {
    lastReadTime = currentTime;
    
    // Print status (use different functions for different levels of detail)
    //modbeeMPPT.printQuickStatus();        // Essential status only - good for testing
    
    // Other debug options available:
     modbeeMPPT.printStatus();            // Complete comprehensive status
    // modbeeMPPT.printPowerMeasurements(); // Just power readings  
    // modbeeMPPT.printConfiguration();     // Just configuration
    // modbeeMPPT.printFaults();           // Just fault status
    // modbeeMPPT.printRegisterDebug();    // Raw register analysis
  }
}
 