#include "ESP8266HTTPClient.h"
#include "HardwareSerial.h"
#include "FirebaseAuth.h"


class FirebaseRTDB {
public:
  static bool deleteDocument(String path, String idToken) {
    return Fetch::DELETE(RTDB_BASE_URL + path + ".json?auth=" + idToken);
  }

  static HttpResponse* updateDocument(String path, String payload, String idToken) {
    return Fetch::PATCH(RTDB_BASE_URL + path + ".json?auth=" + idToken, payload);
  }

  static HttpResponse* createDocument(String path, String payload, String idToken) {
    return Fetch::POST(RTDB_BASE_URL + path + ".json?auth=" + idToken, payload);
  }

  static std::unique_ptr<HttpResponse> onDocumentChange(String path, String idToken, unsigned long ttl, StreamHandler callback) {
    return Fetch::ON(RTDB_BASE_URL + path + ".json?auth=" + idToken, ttl, callback);
  }

  static void ping(String localId, String idToken) {
  }

  static String createDevice(String localId, String idToken) {
    StaticJsonDocument<150> payload;
    JsonObject root = payload.to<JsonObject>();
    JsonObject details = root.createNestedObject("details");
    details["status"] = STATUS_OFF;
    details["state"] = STATE_ACTIVE;
    JsonObject pingObj = details.createNestedObject("ping");
    pingObj[SERVER_VALUE] = FIREBASE_TIMESTAMP;

    String requestBody = "";
    serializeJson(payload, requestBody);

    String deviceID = "";

    HttpResponse* resp = createDocument("users/" + localId + "/devices", requestBody, idToken);

    if (!resp) {
      Serial.println(F("\Error: Invalid Response!"));
      return deviceID;
    }

    int httpStatus = resp->getStatusCode();
    String body = resp->getBody();

    delete resp;

    if (httpStatus != HTTP_CODE_OK) {
      Serial.printf("\nError: invalid status code %d\n", httpStatus);
      return deviceID;
    }

    StaticJsonDocument<100> doc;

    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      Serial.print(F("Failed to parse JSON: "));
      Serial.println(error.c_str());
      return deviceID;
    }

    deviceID = doc["name"].as<String>();

    return deviceID;
  }
};