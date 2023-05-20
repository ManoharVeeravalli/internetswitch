#include "Firestore.h"


class FirebaseRTDB {
public:
  static bool deleteDocument(String path, String idToken) {
    return Fetch::DELETE(RTDB_HOST, RTDB_BASE_URL + path + "?auth=" + idToken, "");
  }

  static HttpResponse* createDocument(String path, String payload, String idToken) {
    return Fetch::POST(RTDB_HOST, RTDB_BASE_URL + path + "?auth=" + idToken, payload, "");
  }

  static HttpResponse* getDocument(String path, String idToken) {
    return Fetch::GET(RTDB_HOST, RTDB_BASE_URL + path + "?auth=" + idToken, "");
  }

  static String createDevice(String localId, String idToken) {
    DynamicJsonDocument payload(150);
    JsonObject root = payload.to<JsonObject>();
    root["status"] = false;
    root["state"] = "Active";

    String deviceID = "";

    HttpResponse* resp = createDocument("users/" + localId + "/devices.json", JSON::stringify(root), idToken);

    if (!resp) {
      return deviceID;
    }

    int httpStatus = resp->getStatusCode();
    String body = resp->getBody();

    delete resp;

    if (httpStatus != HTTP_CODE_OK) {
      return deviceID;
    }

    DynamicJsonDocument* doc = JSON::parse(100, body);

    if (!doc) {
      return deviceID;
    }

    String name = (*doc)["name"].as<String>();

    delete doc;

    return deviceID;
  }
};