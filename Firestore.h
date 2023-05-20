#include "FirebaseAuth.h"


class Firestore {
public:
  static bool deleteDocument(String url, String idToken) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);
    HTTPClient https;

    bool result = false;

    if (https.begin(*client, url)) {

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

  static HttpResponse* createDocument(String url, String payload, String idToken) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);
    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, url)) {

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

  static HttpResponse* getDocument(String url, String idToken) {

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);

    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, url)) {  // HTTPS

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
};