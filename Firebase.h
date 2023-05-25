#include "HardwareSerial.h"
#include "FirebaseRTDB.h"
#include "WiFiClient.h"


class Firebase {
public:


  static bool deleteDeviceFromRTDB(String localId, String deviceId, String idToken) {
    if (!FirebaseRTDB::deleteDocument("users/" + localId + "/devices/" + deviceId, idToken)) {
      Serial.println("Failed to delete document from Firebase RTDB!");
      return false;
    }
    Serial.println("Device Deleted successfully.");
    return true;
  }

  static bool resetRTDB(String localId, String deviceId, String idToken) {
    Serial.printf("\nFree Heap: %d, Heap Fragmentation: %d, Max Block Size: %d \n", ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize());
    Serial.println("\nReceived RESET command removing document from firebase RTDB...");

    if (!deleteDeviceFromRTDB(localId, deviceId, idToken)) {
      return false;
    };

    Serial.println("\nDocument deleted from Firebase RTDB successfully!");

    return reset();
  }

  static void onStatusChangeRTDB(StreamHandler callback) {

    FirebaseConfig* config = getFirebaseConfig();

    if (!config) return;

    String localId = config->getLocalID();
    String deviceId = config->getDeviceID();
    String idToken = config->getToken();
    String refreshToken = config->getRefreshToken();

    delete config;

    String path = "users/" + localId + "/devices/" + deviceId;

    HttpResponse* response = FirebaseRTDB::onDocumentChange(path, idToken, callback);

    if (!response) {
      Serial.println("\nSome Error has Occurred!");
      return;
    }

    int httpCode = response->getStatusCode();
    String body = response->getBody();

    delete response;

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
      Serial.printf("\nRequest Forbidden!");
      return;
    }

    Serial.printf("\nSome Error has Occurred, httpCode: %d", httpCode);
  }


  static bool createDeviceAndSaveConfig(String localId, String idToken, String refreshToken) {


    Serial.println("\ncreating device....");


    String deviceId = createDevice(localId, idToken);


    if (deviceId.isEmpty()) {
      return false;
    }

    Serial.println("device created successfully!");

    if (!saveFirebaseConfig(localId, idToken, refreshToken, deviceId)) {
      deleteDeviceFromRTDB(localId, deviceId, idToken);
      return false;
    }

    Serial.println("\nFirebase configurations saved successfully!");

    return true;
  }


  static String createDevice(String localId, String idToken) {
    return FirebaseRTDB::createDevice(localId, idToken);
  }


  static bool isReady() {
    FirebaseConfig* config = getFirebaseConfig();

    if (!config) return false;

    delete config;
    return true;
  }

  static FirebaseConfig* getFirebaseConfig() {
    DynamicJsonDocument* doc = loadFirebaseConfig();
    if (!doc) {
      return nullptr;
    }


    bool isLocalId = doc->containsKey("localId");
    bool isIdToken = doc->containsKey("idToken");
    bool isRefreshToken = doc->containsKey("refreshToken");
    bool isDeviceId = doc->containsKey("deviceId");


    if (!isLocalId || !isIdToken || !isRefreshToken || !isDeviceId) {
      delete doc;
      return nullptr;
    }


    String localId = (*doc)["localId"].as<String>();
    String idToken = (*doc)["idToken"].as<String>();
    String refreshToken = (*doc)["refreshToken"].as<String>();
    String deviceId = (*doc)["deviceId"].as<String>();

    delete doc;

    if (localId.isEmpty() || idToken.isEmpty() || refreshToken.isEmpty() || deviceId.isEmpty()) {
      return nullptr;
    }

    FirebaseConfig* config = new FirebaseConfig(localId, idToken, refreshToken, deviceId);

    return config;
  }

  static bool saveFirebaseConfig(String& localId, String& idToken, String& refreshToken, String& deviceId) {
    if (LittleFS.exists(FIREBASE_CONFIG_FILE)) {
      if (!LittleFS.remove(FIREBASE_CONFIG_FILE)) {
        Serial.println("Failed to remove config file for writing");
        return false;
      }
    }

    StaticJsonDocument<1536> doc;
    doc["localId"] = localId;
    doc["idToken"] = idToken;
    doc["refreshToken"] = refreshToken;
    doc["deviceId"] = deviceId;


    File configFile = LittleFS.open(FIREBASE_CONFIG_FILE, "w");
    if (!configFile) {
      Serial.println("Failed to open firebase config file for writing");
      return false;
    }

    JSON::stringify(doc, configFile);

    configFile.close();
    return true;
  }

  static bool reset() {
    Serial.println("\nRemoving config files from ESP...");

    if (LittleFS.exists(WIFI_CONFIG_FILE) && !LittleFS.remove(WIFI_CONFIG_FILE)) {
      Serial.println("\nFailed to remove WiFi config!");
      return false;
    }

    Serial.println("\nWiFi config file removed successfully.");

    if (LittleFS.exists(FIREBASE_CONFIG_FILE) && !LittleFS.remove(FIREBASE_CONFIG_FILE)) {
      Serial.println("\nFailed to remove Firebase config!");
      return false;
    }

    Serial.println("\nFirebase config file removed successfully.");

    Serial.println("\nResetting ESP...");

    delay(2000);

    ESP.reset();

    return true;
  }


private:
  static DynamicJsonDocument* loadFirebaseConfig() {
    File configFile = LittleFS.open(FIREBASE_CONFIG_FILE, "r");
    if (!configFile) {
      return nullptr;
    }
    DynamicJsonDocument* doc = JSON::parse(1536, configFile);
    configFile.close();
    return doc;
  }



  static void regerateToken(String refreshToken, String deviceId) {
    Serial.println("\ncredentials have expired, regenrating token!");

    FirebaseConfig* newConfig = FirebaseAuth::regenerateToken(refreshToken);

    if (!newConfig) {
      Serial.println("Failed to generate token!");
    } else {
      Serial.println("\nToken generated successfully, saving to file....");
      String newLocalId = newConfig->getLocalID();
      String newIdToken = newConfig->getToken();
      String newRefreshToken = newConfig->getRefreshToken();

      delete newConfig;

      bool isSaved = saveFirebaseConfig(newLocalId, newIdToken, newRefreshToken, deviceId);
      if (isSaved) {
        Serial.println("Token saved successfully");
      } else {
        Serial.println("Failed to save token!");
      }
    }
  }
};
