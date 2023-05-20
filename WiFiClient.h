#include "LittleFS.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "WiFiConfig.h"

const char* WIFI_CONFIG_FILE = "wifi.json";

class WifiClient {
public:
  static bool isReady() {
    WiFiConfig* config = getWifiConfig();
    if (!config) {
      return false;
    };
    delete config;
    return true;
  }

  static WiFiConfig* getWifiConfig() {
    DynamicJsonDocument* doc = loadWifiConfig();
    if (!doc) {
      return nullptr;
    }
    bool isSsid = doc->containsKey("ssid");
    bool isPassword = doc->containsKey("password");
    if (!isSsid || !isPassword) {
      delete doc;
      return nullptr;
    }
    String ssid = (*doc)["ssid"].as<String>();
    String password = (*doc)["password"].as<String>();

    delete doc;

    if (ssid.isEmpty() || password.isEmpty()) {
      return nullptr;
    }

    WiFiConfig* config = new WiFiConfig(ssid, password);

    return config;
  }


  static bool saveWifiConfig(String ssid, String password) {

    if (LittleFS.exists(WIFI_CONFIG_FILE)) {
      if (!LittleFS.remove(WIFI_CONFIG_FILE)) {
        Serial.println("Failed to remove config file for writing");
        return false;
      }
    }

    StaticJsonDocument<200> doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    File configFile = LittleFS.open(WIFI_CONFIG_FILE, "w");
    if (!configFile) {
      Serial.println("Failed to open config file for writing");
      return false;
    }

    JSON::stringify(doc, configFile);

    configFile.close();
    return true;
  }

  static String scanWiFi() {
    int n = WiFi.scanNetworks();
    DynamicJsonDocument doc(128 * n);
    JsonArray array = doc.to<JsonArray>();
    for (int i = 0; i < n; i++) {
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
    Serial.println("\nDisconnecting from WiFi...");
    WiFi.disconnect(false, true);
  }

  static bool testWifi(String ssid, String password) {
    disconnect();
    delay(100);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    int c = 0;
    Serial.println("Waiting for Wifi to connect: ");
    while (c < 30) {
      if (WiFi.status() == WL_CONNECTED) {
        setupTime();
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
      }
      delay(500);
      Serial.print("*");
      c++;
    }
    Serial.println("");
    Serial.println("Connect timed out");
    return false;
  }

private:
  static DynamicJsonDocument* loadWifiConfig() {
    File configFile = LittleFS.open(WIFI_CONFIG_FILE, "r");
    if (!configFile) {
      return nullptr;
    }
    DynamicJsonDocument* doc = JSON::parse(200, configFile);
    configFile.close();
    return doc;
  }

  static void setupTime() {
    Serial.println();
    // Set time via NTP, as required for x.509 validation
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    Serial.println("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
      delay(500);
      Serial.print("*");
      now = time(nullptr);
    }
    Serial.println("");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));
  }
};