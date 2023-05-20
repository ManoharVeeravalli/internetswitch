#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "HttpResponse.h"

const char* FINGERPRINT = "A7 7B 0F F6 B0 8B 9B CA A7 0B 1A 82 76 10 B2 64 10 BB 17 0A";


class Fetch {

public:
  static bool DELETE(String host, String url, String idToken) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setFingerprint(FINGERPRINT);
    HTTPClient https;
    bool result = false;

    if (https.begin(*client, host, 443, url, true)) {

      if (!idToken.isEmpty()) {
        https.addHeader("Authorization", "Bearer " + idToken);
      }

      int httpCode = https.DELETE();

      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          result = true;
        } else {
          Serial.printf("[HTTPS] Fetch::DELETE DELETE... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
      } else {
        Serial.printf("[HTTPS] Fetch::DELETE DELETE... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] deleteDocument Unable to connect\n");
    }

    return result;
  }


  static HttpResponse* POST(String host, String url, String payload, String idToken) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);
    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, host, 443, url, true)) {
      if (!idToken.isEmpty()) {
        https.addHeader("Authorization", "Bearer " + idToken);
      }
      int httpCode = https.POST(payload);

      if (httpCode > 0) {
        String payload = https.getString();
        response = new HttpResponse(httpCode, payload);
      } else {
        Serial.printf("[HTTPS] Fetch::POST POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] createDocument Unable to connect\n");
    }

    return response;
  }

  static HttpResponse* GET(String host, String url, String idToken) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);

    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, host, 443, url, true)) {  // HTTPS

      if (!idToken.isEmpty()) {
        https.addHeader("Authorization", "Bearer " + idToken);
      }
      int httpCode = https.GET();

      if (httpCode > 0) {
        String payload = https.getString();
        response = new HttpResponse(httpCode, payload);
      } else {
        Serial.printf("[HTTPS] Fetch::GET GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Fetch::GET Unable to connect\n");
    }

    return response;
  }
};