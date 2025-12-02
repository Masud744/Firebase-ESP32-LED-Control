#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
// Include the correct WiFi header based on the board
#if defined(ESP32)
    #include <WiFi.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h> // Provides the official tokenStatusCallback
// ArduinoJson is no longer strictly needed but kept for general Firebase use, 
// though the JSON parsing logic is removed from streamCallback.
#include <ArduinoJson.h>       

// --------------------- USER CONFIG ---------------------
// Defined as const char* in global scope for proper configuration access
const char* WIFI_SSID     = "BDU-Hostel2";
const char* WIFI_PASSWORD = "dhaka1213";

const char* WEB_API_KEY   = "AIzaSyBarD8sXgMkk264LDZtBM1yTzKOIPUp_tU";
const char* DATABASE_URL  = "https://led-control-6e138-default-rtdb.asia-southeast1.firebasedatabase.app"; 
const char* USER_EMAIL    = "masud.nil74@gmail.com";
const char* USER_PASS     = "123456";
// --------------------------------------------------------

// Firebase objects
FirebaseData fbdo;    // Realtime database object
FirebaseAuth auth;    // User authentication object
FirebaseConfig config;

// GPIO pins (Only GPIO1 is physically used for the LED)
const int GPIO1 = 12; // Your connected LED (GPIO 12)
const int GPIO2 = 13; // Unused
const int GPIO3 = 14; // Unused

// Forward declarations
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
// tokenStatusCallback is provided by TokenHelper.h

// Connect WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // Setup GPIO pins - ONLY GPIO1 is initialized
  pinMode(GPIO1, OUTPUT);
  digitalWrite(GPIO1, LOW); 

  initWiFi();

  // Firebase config
  config.api_key = WEB_API_KEY;
  config.database_url = DATABASE_URL; 
  
  // Auth setup
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASS;
  config.token_status_callback = tokenStatusCallback;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Start streaming ONLY from /gpio1 for reliable single-pin control
  if (!Firebase.RTDB.beginStream(&fbdo, "/gpio1")) { // <-- The fix for reliable control
    Serial.printf("Stream begin error, %s\n\n", fbdo.errorReason().c_str());
  }

  Firebase.RTDB.setStreamCallback(&fbdo, streamCallback, streamTimeoutCallback);

  Serial.println("Firebase streaming initialized...");
}

void loop() {
  // Nothing needed here, streaming is handled in background
}

// --------------------- STREAM CALLBACK (Simplified & Reliable) ---------------------
void streamCallback(FirebaseStream data) {
  Serial.println("----- Stream Update -----");
  Serial.printf("Event Type: %s\n", data.eventType().c_str());
  Serial.printf("Path: %s\n", data.streamPath().c_str());
  Serial.printf("Data: %s\n", data.stringData().c_str());

  // Since we are streaming only /gpio1, we can rely on the simple value.
  if (data.streamPath() == "/gpio1") {
      String value = data.stringData();
      int intValue = value.toInt(); 

      // Control the LED on GPIO1 (Pin 12)
      digitalWrite(GPIO1, intValue ? HIGH : LOW);
      Serial.printf("✅ GPIO1 (Pin 12) set to %s\n", intValue ? "HIGH" : "LOW");
  } else {
      Serial.println("Warning: Received data on unexpected path. Ignoring.");
  }

  Serial.println("GPIO Updated.");
  Serial.println("-------------------------");
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream timeout, resuming...");
  }
}