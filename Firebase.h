#include "HardwareSerial.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "FirebaseConfig.h"
#include "HttpResponse.h"
#include "WiFiClient.h"


const char* FIREBASE_CONFIG_FILE = "firebase.json";

const char* FINGERPRINT = "A7 7B 0F F6 B0 8B 9B CA A7 0B 1A 82 76 10 B2 64 10 BB 17 0A";

const char* LOGIN_URL = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=AIzaSyD0o2HHGWp6oP_VTgA5DA4liDGAvXzIOYE";

const char* REFRESH_TOKEN_URL = "https://securetoken.googleapis.com/v1/token?key=AIzaSyD0o2HHGWp6oP_VTgA5DA4liDGAvXzIOYE";


String FIRESTORE_BASE_URL = "https://firestore.googleapis.com/v1/projects/metrix-3c2e5/databases/(default)/documents/";



String HOST = "firestore.googleapis.com";



class Firebase {
public:
  static HttpResponse* signInWithEmailAndPassword(String email, String password) {
    DynamicJsonDocument doc(200);
    doc["email"] = email;
    doc["password"] = password;
    doc["returnSecureToken"] = true;

    String payload = JSON::stringify(doc);

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);

    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, HOST, 443, LOGIN_URL, true)) {

      int httpCode = https.POST(payload);

      String payload = https.getString();
      if (httpCode > 0) {
        response = new HttpResponse(httpCode, payload);
      } else {
        Serial.printf("[HTTPS] signInWithEmailAndPassword POST... failed, error: %d\n", httpCode);
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] signInWithEmailAndPassword Unable to connect\n");
    }

    return response;
  }

  static String getStatusFromFirebase() {

    String result = "";

    FirebaseConfig* config = getFirebaseConfig();

    if (!config) return result;

    String localId = config->getLocalID();
    String deviceId = config->getDeviceID();
    String idToken = config->getToken();
    String refreshToken = config->getRefreshToken();

    delete config;

    String path = "users/" + localId + "/devices/" + deviceId;

    HttpResponse* response = getDocumentFromFirestore(path, idToken);

    if (!response) {
      Serial.println("Some Error has Occurred!");
      return result;
    }

    int httpCode = response->getStatusCode();
    String body = response->getBody();

    delete response;

    if (httpCode != HTTP_CODE_OK) {

      if (httpCode == HTTP_CODE_UNAUTHORIZED) {

        Serial.println("credentials have expired, regenrating token!");

        if (regenerateToken(deviceId, refreshToken)) {
          Serial.println("Token regenerated successfully!");
        } else {
          Serial.println("Failed to generate token!");
        }

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

      if (!deleteDeviceDocument(deviceId, localId, idToken)) {
        Serial.println("\nFailed to delete document from Firebase!");
        return result;
      }

      Serial.println("\nDocument deleted from firebase successfully!");

      Serial.println("\nRemoving config files from ESP...");

      if (LittleFS.exists(WIFI_CONFIG_FILE) && !LittleFS.remove(WIFI_CONFIG_FILE)) {
        Serial.println("\nFailed to remove WiFi config!");
        return result;
      }

      Serial.println("\nWiFi config file removed successfully.");

      if (LittleFS.exists(FIREBASE_CONFIG_FILE) && !LittleFS.remove(FIREBASE_CONFIG_FILE)) {
        Serial.println("\nFailed to remove Firebase config!");
        return result;
      }

      Serial.println("\nFirebase config file removed successfully.");

      Serial.println("\nResetting ESP...");

      delay(2000);

      ESP.reset();

      return result;
    }

    result = status ? "HIGH" : "LOW";

    return result;
  }

  static HttpResponse* getDocumentFromFirestore(String path, String idToken) {

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);

    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, FIRESTORE_BASE_URL + path)) {  // HTTPS

      https.addHeader("Authorization", "Bearer " + idToken);
      int httpCode = https.GET();

      if (httpCode > 0) {
        String payload = https.getString();
        response = new HttpResponse(httpCode, payload);
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }

    return response;
  }

  static bool regenerateToken(String deviceId, String refreshToken) {
    DynamicJsonDocument doc(384);
    doc["grant_type"] = "refresh_token";
    doc["refresh_token"] = refreshToken;
    String payload = JSON::stringify(doc);

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);

    HTTPClient https;

    bool result = false;

    if (https.begin(*client, HOST, 443, REFRESH_TOKEN_URL, true)) {

      // start connection and send HTTP header
      int httpCode = https.POST(payload);

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        if (httpCode == HTTP_CODE_OK) {
          String output = https.getString();
          DynamicJsonDocument* doc = JSON::parse(3072, output);
          if (doc) {
            String newLocalId = (*doc)["user_id"].as<String>();
            String newIdToken = (*doc)["id_token"].as<String>();
            String newRefreshToken = (*doc)["refresh_token"].as<String>();

            delete doc;

            bool isSaved = saveFirebaseConfig(newLocalId, newIdToken, newRefreshToken, deviceId);
            result = isSaved;
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
      } else {
        Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }

    return result;
  }

  static HttpResponse* createDeviceDocument(String localId, String idToken) {
    DynamicJsonDocument doc(150);
    JsonObject root = doc.to<JsonObject>();
    JsonObject fieldsObj = root.createNestedObject("fields");
    JsonObject statusObj = fieldsObj.createNestedObject("status");
    statusObj["booleanValue"] = false;
    JsonObject stateObj = fieldsObj.createNestedObject("state");
    stateObj["stringValue"] = "ACTIVE";

    String payload = JSON::stringify(root);

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);
    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, FIRESTORE_BASE_URL + "users/" + localId + "/devices")) {

      https.addHeader("Authorization", "Bearer " + idToken);
      int httpCode = https.POST(payload);

      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = https.getString();
          response = new HttpResponse(httpCode, payload);
        } else {
          Serial.printf("[HTTPS] createDeviceDocument GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
      } else {
        Serial.printf("[HTTPS] createDeviceDocument GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] createDeviceDocument Unable to connect\n");
    }

    return response;
  }

  static bool deleteDeviceDocument(String deviceId, String localId, String idToken) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);
    HTTPClient https;

    bool result = false;

    if (https.begin(*client, FIRESTORE_BASE_URL + "users/" + localId + "/devices/" + deviceId)) {

      https.addHeader("Authorization", "Bearer " + idToken);
      int httpCode = https.DELETE();

      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          result = true;
        } else {
          Serial.printf("[HTTPS] deleteDeviceDocument GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
      } else {
        Serial.printf("[HTTPS] deleteDeviceDocument GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] deleteDeviceDocument Unable to connect\n");
    }

    return result;
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
};
