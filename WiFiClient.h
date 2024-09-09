#include "wl_definitions.h"
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "WiFiConfig.h"

const char* WIFI_CONFIG_FILE = "wifi.json";
const int WIFI_CONNECT_TIMEOUT = 30;  // Timeout in 30 iterations (15 seconds)
const int MAX_WIFI_NETWORKS = 10;     // Limit the number of Wi-Fi networks processed

class WifiClient {
public:

  static bool isReady() {
    return getWifiConfig().isValid();
  }

  static WiFiConfig getWifiConfig() {
    return loadWifiConfig();
  }

  static bool saveWifiConfig(String ssid, String password) {

    if (!LittleFS.begin()) {  // Ensure LittleFS is mounted
      return false;
    }

    // Validate SSID and Password before saving
    if (ssid.length() < 1 || password.length() < 8) {  // Wi-Fi passwords need to be at least 8 characters
      Serial.println(F("Invalid WiFi credentials"));
      return false;
    }

    if (LittleFS.exists(WIFI_CONFIG_FILE)) {
      if (!LittleFS.remove(WIFI_CONFIG_FILE)) {
        Serial.println(F("Failed to remove config file for writing"));
        return false;
      }
    }

    StaticJsonDocument<512> doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    File configFile = LittleFS.open(WIFI_CONFIG_FILE, "w");
    if (!configFile) {
      Serial.println(F("Failed to open config file for writing"));
      return false;
    }

    // Corrected JSON serialization function
    serializeJson(doc, configFile);

    configFile.close();
    return true;
  }

  static String scanWiFi() {
    int n = WiFi.scanNetworks();
    if (n == 0) {
      Serial.println(F("No networks found"));
      return "{}";
    }

    // Limit the number of networks processed
    n = min(n, MAX_WIFI_NETWORKS);

    DynamicJsonDocument doc(128 * n);
    JsonArray array = doc.to<JsonArray>();
    for (int i = 0; i < n; i++) {
      if (WiFi.encryptionType(i) == ENC_TYPE_NONE) { //we dont want to show unsecured networks
        continue;
      }
      JsonObject nested = array.createNestedObject();
      nested["ssid"] = WiFi.SSID(i);
      nested["rssi"] = WiFi.RSSI(i);
      nested["encryptionType"] = WiFi.encryptionType(i);
    }
    String json = "";
    serializeJson(array, json);
    return json;
  }

  static void disconnect() {
    Serial.println(F("\nDisconnecting from WiFi..."));
    WiFi.disconnect(false, true);
  }

  static bool testWifi(String ssid, String password) {
    disconnect();
    delay(100);
    Serial.println();
    Serial.print(F("Connecting to "));
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    unsigned long startTime = millis();

    Serial.println(F("Waiting for WiFi to connect: "));
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startTime > WIFI_CONNECT_TIMEOUT * 500) {  // Timeout after 15 seconds
        Serial.println(F("Connect timed out"));
        return false;
      }
      delay(500);
    }

    setupTime();
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());
    return true;
  }

private:

  static WiFiConfig loadWifiConfig() {
    DynamicJsonDocument doc(200);
    WiFiConfig config;

    if (!LittleFS.begin()) {  // Ensure LittleFS is mounted
      return config;
    }

    File configFile = LittleFS.open(WIFI_CONFIG_FILE, "r");
    if (!configFile) {
      return config;
    }

    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    if (error) {
      Serial.println(F("Failed to read wifi config file"));
      return config;
    }

    bool isSsid = doc.containsKey("ssid");
    bool isPassword = doc.containsKey("password");
    if (!isSsid || !isPassword) {
      Serial.println(F("Invalid wifi config file structure"));
      return config;
    }

    String ssid = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();

    if (ssid.isEmpty() || password.isEmpty()) {
      Serial.println(F("wifi config values are empty"));
      return config;
    }

    config = WiFiConfig(ssid, password);
    return config;
  }

  static void setupTime() {
    Serial.println();
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    Serial.println(F("Waiting for NTP time sync: "));
    time_t now = time(nullptr);
    unsigned long startTime = millis();

    while (now < 8 * 3600 * 2) {
      if (millis() - startTime > 15000) {  // Timeout after 15 seconds
        Serial.println(F("NTP sync timed out"));
        break;
      }
      delay(500);
      now = time(nullptr);
    }

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print(F("Current time: "));
    Serial.print(asctime(&timeinfo));
  }
};
