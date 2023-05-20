#include "HardwareSerial.h"
#include "Firestore.h"
#include "WiFiClient.h"


class Firebase {
public:
  static String getStatusFromFirestore() {

    String result = "";

    FirebaseConfig* config = getFirebaseConfig();

    if (!config) return result;

    String localId = config->getLocalID();
    String deviceId = config->getDeviceID();
    String idToken = config->getToken();
    String refreshToken = config->getRefreshToken();

    delete config;

    HttpResponse* response = Firestore::getDocument("users/" + localId + "/devices/" + deviceId, idToken);

    if (!response) {
      Serial.println("Some Error has Occurred!");
      return result;
    }

    int httpCode = response->getStatusCode();
    String body = response->getBody();

    delete response;

    if (httpCode != HTTP_CODE_OK) {
      if (httpCode == HTTP_CODE_UNAUTHORIZED) {
        regerateToken(refreshToken, deviceId);
      } else {
        Serial.printf("Some Error has Occurred, httpCode: %d", httpCode);
      }
      return result;
    }

    DynamicJsonDocument* doc = JSON::parse(384, body);

    if (!doc) {
      Serial.println("Some Error has Occurred!");
      return result;
    }

    const bool status = (*doc)["fields"]["status"]["booleanValue"].as<bool>();
    String state = (*doc)["fields"]["state"]["stringValue"].as<String>();


    delete doc;

    if (state && state == "RESET") {
      Serial.println("\nReceived RESET command removing document from firebase...");

      if (!Firestore::deleteDocument("users/" + localId + "/devices/" + deviceId, idToken)) {
        Serial.println("\nFailed to delete document from Firebase!");
        return result;
      }

      Serial.println("\nDocument deleted from firebase successfully!");
      reset();

      return result;
    }

    result = status ? "HIGH" : "LOW";

    return result;
  }


  static HttpResponse* createFirestoreDocument(String localId, String idToken) {
    DynamicJsonDocument doc(150);
    JsonObject root = doc.to<JsonObject>();
    JsonObject fieldsObj = root.createNestedObject("fields");
    JsonObject statusObj = fieldsObj.createNestedObject("status");
    statusObj["booleanValue"] = false;
    JsonObject stateObj = fieldsObj.createNestedObject("state");
    stateObj["stringValue"] = "ACTIVE";
    String payload = JSON::stringify(root);

    return Firestore::createDocument("users/" + localId + "/devices", payload, idToken);
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

  static String getDeviceIDFromName(String& name) {
    int index = indexOfReverse(name, '/');
    return name.substring(index + 1, name.length());
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

  static int indexOfReverse(String str, char c) {
    for (int i = str.length() - 1; i >= 0; i--) {
      if (str[i] == c) return i;
    }
    return -1;
  }

  static void regerateToken(String refreshToken, String deviceId) {
    Serial.println("credentials have expired, regenrating token!");

    FirebaseConfig* newConfig = FirebaseAuth::regenerateToken(refreshToken);

    if (!newConfig) {
      Serial.println("Failed to generate token!");
    } else {
      Serial.println("Token generated successfully, saving to file....");
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

  static void reset() {
    Serial.println("\nRemoving config files from ESP...");

    if (LittleFS.exists(WIFI_CONFIG_FILE) && !LittleFS.remove(WIFI_CONFIG_FILE)) {
      Serial.println("\nFailed to remove WiFi config!");
      return;
    }

    Serial.println("\nWiFi config file removed successfully.");

    if (LittleFS.exists(FIREBASE_CONFIG_FILE) && !LittleFS.remove(FIREBASE_CONFIG_FILE)) {
      Serial.println("\nFailed to remove Firebase config!");
      return;
    }

    Serial.println("\nFirebase config file removed successfully.");

    Serial.println("\nResetting ESP...");

    delay(2000);

    ESP.reset();
  }
};
