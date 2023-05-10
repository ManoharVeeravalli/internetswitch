#include "HardwareSerial.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "FirebaseConfig.h"
#include "HttpResponse.h"

const char* FIREBASE_CONFIG_FILE = "firebase.json";

std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

const char* FINGERPRINT = "A3 91 34 22 BC 1A 48 69 72 56 FE 52 F7 DA 7D 9A 46 1B DA 05";

const char* LOGIN_URL = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=AIzaSyD0o2HHGWp6oP_VTgA5DA4liDGAvXzIOYE";

String FIRESTORE_BASE_URL = "https://firestore.googleapis.com/v1/projects/metrix-3c2e5/databases/(default)/documents";



String HOST = "firestore.googleapis.com";



class Firebase {
public:
  static HttpResponse* signInWithEmailAndPassword(String email, String password) {
    DynamicJsonDocument doc(200);
    doc["email"] = email;
    doc["password"] = password;
    doc["returnSecureToken"] = true;

    String payload = JSON::stringify(doc);

    client->setFingerprint(FINGERPRINT);

    HTTPClient https;

    HttpResponse* response;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, HOST, 443, LOGIN_URL, true)) {

      Serial.print("[HTTPS] POST...\n");
      // start connection and send HTTP header
      int httpCode = https.POST(payload);

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

        String payload = https.getString();
        Serial.println(payload);
        response = new HttpResponse(httpCode, payload);
      } else {
        Serial.println(httpCode);
        Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }

    return response;
  }

  static String getStatusFromFirebase() {
    String result = "";
    client->setFingerprint(FINGERPRINT);

    HTTPClient https;

    FirebaseConfig* config = getFirebaseConfig();

    if (!config) return result;

    String localId = config->getLocalID();
    String deviceId = config->getDeviceID();
    String idToken = config->getToken();

    delete config;


    if (https.begin(*client, FIRESTORE_BASE_URL + "/users/" + localId + "/devices/" + deviceId)) {  // HTTPS

      https.addHeader("Authorization", "Bearer " + idToken);
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        if (httpCode == HTTP_CODE_OK) {
          String payload = https.getString();
          DynamicJsonDocument* doc = JSON::parse(384, payload);
          if (doc) {
            const bool status = (*doc)["fields"]["status"]["booleanValue"].as<bool>();
            result = status ? "HIGH" : "LOW";
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }

    return result;
  }

  static HttpResponse* createDeviceDocument(String localId, String idToken) {
    DynamicJsonDocument doc(96);
    JsonObject root = doc.to<JsonObject>();
    JsonObject fieldsObj = root.createNestedObject("fields");
    JsonObject statusObj = fieldsObj.createNestedObject("status");
    statusObj["booleanValue"] = false;

    String payload = JSON::stringify(root);

    Serial.println();
    Serial.println(payload);
    Serial.println();

    client->setFingerprint(FINGERPRINT);
    HTTPClient https;

    HttpResponse* response;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, FIRESTORE_BASE_URL + "/users/" + localId + "/devices")) {  // HTTPS

      Serial.print("[HTTPS] POST...\n");
      https.addHeader("Authorization", "Bearer " + idToken);
      // start connection and send HTTP header
      int httpCode = https.POST(payload);

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
          String payload = https.getString();
          response = new HttpResponse(httpCode, payload);
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }

    return response;
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

  static bool saveFirebaseConfig(String localId, String idToken, String refreshToken, String deviceId) {
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

    serializeJson(doc, configFile);
    return true;
  }

  static String getDeviceIDFromName(String name) {
    int index = indexOfReverse(name, '/');
    return name.substring(index + 1, name.length());
  }
private:
  static DynamicJsonDocument* loadFirebaseConfig() {
    File configFile = LittleFS.open(FIREBASE_CONFIG_FILE, "r");
    if (!configFile) {
      return nullptr;
    }

    DynamicJsonDocument* doc = new DynamicJsonDocument(1536);
    auto error = deserializeJson(*doc, configFile);
    if (error) {
      return nullptr;
    }
    return doc;
  }

  static int indexOfReverse(String str, char c) {
    for (int i = str.length() - 1; i >= 0; i--) {
      if (str[i] == c) return i;
    }
    return -1;
  }
};
