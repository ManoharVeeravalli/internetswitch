#include "FirebaseAuth.h"


class Firestore {
public:
  static bool deleteDocument(String path, String idToken) {
    return Fetch::DELETE(FIRESTORE_HOST, FIRESTORE_BASE_URL + path, idToken);
  }

  static HttpResponse* createDocument(String path, String payload, String idToken) {
    return Fetch::POST(FIRESTORE_HOST, FIRESTORE_BASE_URL + path, payload, idToken);
  }

  static HttpResponse* getDocument(String path, String idToken) {
    return Fetch::GET(FIRESTORE_HOST, FIRESTORE_BASE_URL + path, idToken);
  }

  static String createDevice(String localId, String idToken) {
    DynamicJsonDocument payload(150);
    JsonObject root = payload.to<JsonObject>();
    JsonObject fieldsObj = root.createNestedObject("fields");
    JsonObject statusObj = fieldsObj.createNestedObject("status");
    statusObj["stringValue"] = STATUS_OFF;
    JsonObject stateObj = fieldsObj.createNestedObject("state");
    stateObj["stringValue"] = STATE_ACTIVE;

    String deviceID = "";

    HttpResponse* resp = createDocument("users/" + localId + "/devices", JSON::stringify(root), idToken);

    if (!resp) {
      return deviceID;
    }

    int httpStatus = resp->getStatusCode();
    String body = resp->getBody();

    delete resp;

    if (httpStatus != HTTP_CODE_OK) {
      return deviceID;
    }

    DynamicJsonDocument* doc = JSON::parse(384, body);

    if (!doc) {
      return deviceID;
    }

    String name = (*doc)["name"].as<String>();

    delete doc;

    deviceID = getDeviceIDFromName(name);

    return deviceID;
  }

private:
  static String getDeviceIDFromName(String& name) {
    int index = StringUtils::indexOfReverse(name, '/');
    return name.substring(index + 1, name.length());
  }
};