#include "ESP8266HTTPClient.h"
#include "HardwareSerial.h"
#include "FirebaseAuth.h"


class FirebaseRTDB {
public:
  static bool deleteDocument(String path, String idToken) {
    return Fetch::DELETE(RTDB_HOST, "/" + path + ".json?auth=" + idToken, "");
  }

  static HttpResponse* createDocument(String path, String payload, String idToken) {
    return Fetch::POST(RTDB_HOST, "/" + path + ".json?auth=" + idToken, payload, "");
  }

  static HttpResponse* getDocument(String path, String idToken) {
    return Fetch::GET(RTDB_HOST, "/" + path + ".json?auth=" + idToken, "");
  }

  static HttpResponse* onDocumentChange(String path, String idToken, StreamHandler callback) {
    return Fetch::ON(RTDB_HOST, "/" + path + ".json?auth=" + idToken, "", callback);
  }

  static String createDevice(String localId, String idToken) {
    DynamicJsonDocument payload(150);
    JsonObject root = payload.to<JsonObject>();
    root["status"] = STATUS_OFF;
    root["state"] = STATE_ACTIVE;

    String deviceID = "";

    HttpResponse* resp = createDocument("users/" + localId + "/devices", JSON::stringify(root), idToken);

    if (!resp) {
      Serial.println("\Error: Invalid Response!");
      return deviceID;
    }

    int httpStatus = resp->getStatusCode();
    String body = resp->getBody();

    delete resp;

    if (httpStatus != HTTP_CODE_OK) {
      Serial.printf("\nError: invalid status code %d\n", httpStatus);
      return deviceID;
    }

    DynamicJsonDocument* doc = JSON::parse(100, body);

    if (!doc) {
      return deviceID;
    }

    deviceID = (*doc)["name"].as<String>();

    delete doc;

    return deviceID;
  }
};