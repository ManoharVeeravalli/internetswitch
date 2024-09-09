#include "HardwareSerial.h"
#include "FirebaseRTDB.h"
#include "WiFiClient.h"


class Firebase {
public:


  static bool deleteDeviceFromRTDB(String localId, String deviceId, String idToken) {
    if (!FirebaseRTDB::deleteDocument("users/" + localId + "/devices/" + deviceId, idToken)) {
      Serial.println(F("Failed to delete document from Firebase RTDB!"));
      return false;
    }
    Serial.println(F("Device Deleted successfully."));
    return true;
  }

  static bool resetRTDB(String localId, String deviceId, String idToken) {
    Serial.printf("\nFree Heap: %d, Heap Fragmentation: %d, Max Block Size: %d \n", ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize());
    Serial.println(F("\nReceived RESET command removing document from firebase RTDB..."));

    if (!deleteDeviceFromRTDB(localId, deviceId, idToken)) {
      return false;
    };

    Serial.println(F("\nDocument deleted from Firebase RTDB successfully!"));

    return reset();
  }

  static void onStatusChangeRTDB(unsigned long ttl, StreamHandler callback) {

    FirebaseConfig config = getFirebaseConfig();

    if (!config.isValid()) return;

    String localId = config.getLocalID();
    String deviceId = config.getDeviceID();
    String idToken = config.getToken();
    String refreshToken = config.getRefreshToken();

    String path = "users/" + localId + "/devices/" + deviceId + "/details";

    std::unique_ptr<HttpResponse> response = FirebaseRTDB::onDocumentChange(path, idToken, ttl, callback);

    if (!response) {
      Serial.println(F("\nSome Error has Occurred!"));
      return;
    }

    int httpCode = response->getStatusCode();
    String body = response->getBody();

    if (httpCode == HTTP_CODE_OK) {
      return;
    }

    if (httpCode == HTTP_CODE_UNAUTHORIZED) {
      regerateToken(refreshToken, deviceId);
      return;
    }

    if (httpCode == HTTP_CODE_RESET_CONTENT) {
      resetRTDB(localId, deviceId, idToken);
      return;
    }

    if (httpCode == HTTP_CODE_FORBIDDEN) {
      Serial.println(F("\nRequest Forbidden!"));
      return;
    }

    Serial.printf("\nSome Error has Occurred, httpCode: %d", httpCode);
  }

  static void ping() {
    Serial.println(F("\npinging...."));

    FirebaseConfig config = getFirebaseConfig();
    if (!config.isValid()) return;

    String localId = config.getLocalID();
    String deviceId = config.getDeviceID();
    String idToken = config.getToken();
    String refreshToken = config.getRefreshToken();

    StaticJsonDocument<200> payload;

    JsonObject root = payload.to<JsonObject>();

    JsonObject pingObj = root.createNestedObject("ping");

    pingObj[SERVER_VALUE] = FIREBASE_TIMESTAMP;

    String requestBody = "";

    serializeJson(payload, requestBody);

    HttpResponse* response = FirebaseRTDB::updateDocument("users/" + localId + "/devices/" + deviceId + "/details", requestBody, idToken);
    int statusCode = response->getStatusCode();
    String body = response->getBody();
    delete response;

    if (statusCode == HTTP_CODE_OK) {
      Serial.println(F("ping successful!"));
      return;
    }

    if (statusCode == HTTP_CODE_UNAUTHORIZED) {
      Serial.println(F("Auth token expired!"));
      if(regerateToken(refreshToken, deviceId)) {
        ping();
      }
      return;
    }

    Serial.println(F("Failed to ping"));
  }

  static void recordDeviceHistory(String message) {
    Serial.println(F("\nrecoding device history...."));
    FirebaseConfig config = getFirebaseConfig();

    if (!config.isValid()) return;

    String localId = config.getLocalID();
    String deviceId = config.getDeviceID();
    String idToken = config.getToken();
    String refreshToken = config.getRefreshToken();

    StaticJsonDocument<200> payload;
    JsonObject root = payload.to<JsonObject>();
    root["message"] = message;
    JsonObject createdAt = root.createNestedObject("createdAt");
    createdAt[SERVER_VALUE] = FIREBASE_TIMESTAMP;

    String requestBody = "";
    serializeJson(payload, requestBody);

    HttpResponse* response = FirebaseRTDB::createDocument("users/" + localId + "/devices/" + deviceId + "/history", requestBody, idToken);
    int statusCode = response->getStatusCode();
    String body = response->getBody();
    delete response;

    if (statusCode == HTTP_CODE_OK) {
      Serial.println(F("History recorded successfully"));
      return;
    }
    if (statusCode == HTTP_CODE_UNAUTHORIZED) {
      Serial.println(F("Auth token expired!"));
      if(regerateToken(refreshToken, deviceId)) {
        recordDeviceHistory(message);
      }
      return;
    }
    Serial.println(F("Failed to record history"));
  }

  static bool createDeviceAndSaveConfig(String localId, String idToken, String refreshToken) {


    Serial.println(F("\ncreating device...."));


    String deviceId = createDevice(localId, idToken);


    if (deviceId.isEmpty()) {
      return false;
    }

    Serial.println(F("device created successfully!"));

    if (!saveFirebaseConfig(localId, idToken, refreshToken, deviceId)) {
      deleteDeviceFromRTDB(localId, deviceId, idToken);
      return false;
    }

    Serial.println(F("\nFirebase configurations saved successfully!"));

    return true;
  }


  static String createDevice(String localId, String idToken) {
    return FirebaseRTDB::createDevice(localId, idToken);
  }


  static bool isReady() {
    FirebaseConfig config = getFirebaseConfig();
    return config.isValid();
  }

  static FirebaseConfig getFirebaseConfig() {
    FirebaseConfig config;
    File configFile = LittleFS.open(FIREBASE_CONFIG_FILE, "r");
    if (!configFile) {
      return config;
    }
    auto doc = std::make_unique<DynamicJsonDocument>(1536);
    DeserializationError error = deserializeJson(*doc, configFile);
    configFile.close();

    if (error) {
        Serial.print(F("Failed to parse JSON: "));
        Serial.println(error.c_str());
        return config;
    }

    bool hasLocalId = doc->containsKey(LOCAL_ID);
    bool hasIdToken = doc->containsKey(ID_TOKEN);
    bool hasRefreshToken = doc->containsKey(REFRESH_TOKEN);
    bool hasDeviceId = doc->containsKey(DEVICE_ID);


    if (!hasLocalId || !hasIdToken || !hasRefreshToken || !hasDeviceId) {
      return config;
    }


    String localId = (*doc)[LOCAL_ID].as<String>();
    String idToken = (*doc)[ID_TOKEN].as<String>();
    String refreshToken = (*doc)[REFRESH_TOKEN].as<String>();
    String deviceId = (*doc)[DEVICE_ID].as<String>();

    if (localId.isEmpty() || idToken.isEmpty() || refreshToken.isEmpty() || deviceId.isEmpty()) {
      return config;
    }

    config = FirebaseConfig(localId, idToken, refreshToken, deviceId);

    return config;
  }

  static bool saveFirebaseConfig(String& localId, String& idToken, String& refreshToken, String& deviceId) {
    if (LittleFS.exists(FIREBASE_CONFIG_FILE)) {
      if (!LittleFS.remove(FIREBASE_CONFIG_FILE)) {
        Serial.println(F("Failed to remove config file for writing"));
        return false;
      }
    }

    StaticJsonDocument<1536> doc;
    doc[LOCAL_ID] = localId;
    doc[ID_TOKEN] = idToken;
    doc[REFRESH_TOKEN] = refreshToken;
    doc[DEVICE_ID] = deviceId;


    File configFile = LittleFS.open(FIREBASE_CONFIG_FILE, "w");
    if (!configFile) {
      Serial.println(F("Failed to open firebase config file for writing"));
      return false;
    }

    serializeJson(doc, configFile);

    configFile.close();
    return true;
  }

  static bool reset() {
    Serial.println(F("\nRemoving config files from ESP..."));

    if (LittleFS.exists(WIFI_CONFIG_FILE) && !LittleFS.remove(WIFI_CONFIG_FILE)) {
      Serial.println(F("\nFailed to remove WiFi config!"));
      return false;
    }

    Serial.println(F("\nWiFi config file removed successfully."));

    if (LittleFS.exists(FIREBASE_CONFIG_FILE) && !LittleFS.remove(FIREBASE_CONFIG_FILE)) {
      Serial.println(F("\nFailed to remove Firebase config!"));
      return false;
    }

    Serial.println(F("\nFirebase config file removed successfully."));

    Serial.println(F("\nResetting ESP..."));

    delay(2000);

    ESP.reset();

    return true;
  }


private:

  static bool regerateToken(String refreshToken, String deviceId) {
    Serial.println(F("\ncredentials have expired, regenrating token!"));

    FirebaseConfig newConfig = FirebaseAuth::regenerateToken(refreshToken);

    if (!newConfig.isValid()) {
      Serial.println(F("Failed to generate token!"));
      return false;
    }

    Serial.println(F("\nToken generated successfully, saving to file...."));
    String newLocalId = newConfig.getLocalID();
    String newIdToken = newConfig.getToken();
    String newRefreshToken = newConfig.getRefreshToken();

    bool isSaved = saveFirebaseConfig(newLocalId, newIdToken, newRefreshToken, deviceId);
    if (isSaved) {
      Serial.println(F("Token saved successfully"));
    } else {
      Serial.println(F("Failed to save token!"));
    }
    return isSaved;
  }
};
