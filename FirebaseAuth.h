#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "HttpResponse.h"
#include "FirebaseConfig.h"
#include "FirebaseConstants.h"


class FirebaseAuth {

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

  static FirebaseConfig* regenerateToken(String refreshToken) {
    DynamicJsonDocument doc(384);
    doc["grant_type"] = "refresh_token";
    doc["refresh_token"] = refreshToken;
    String payload = JSON::stringify(doc);

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(FINGERPRINT);

    HTTPClient https;

    FirebaseConfig* response = nullptr;

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
            String deviceId = "";

            delete doc;

            response = new FirebaseConfig(newLocalId, newIdToken, newRefreshToken, deviceId);
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

    return response;
  }
};