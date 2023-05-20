#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "HttpResponse.h"

const char* FINGERPRINT = "A7 7B 0F F6 B0 8B 9B CA A7 0B 1A 82 76 10 B2 64 10 BB 17 0A";

const char* RTDB_FINGERPRINT = "91 14 41 84 C3 F8 48 9D 29 56 8C D4 35 43 F6 B8 53 F1 FE FE";

typedef std::function<void(String)> StreamHandler;

class Fetch {

public:
  static bool DELETE(String host, String url, String idToken) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setFingerprint(getFingreprint(host));
    HTTPClient https;
    bool result = false;

    Serial.println(url);

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

    client->setFingerprint(getFingreprint(host));
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
      Serial.printf("[HTTPS]  Fetch::POST  Unable to connect\n");
    }

    return response;
  }

  static HttpResponse* GET(String host, String url, String idToken) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(getFingreprint(host));

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

  static HttpResponse* ON(String host, String url, String idToken, StreamHandler handler) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(getFingreprint(host));

    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, host, 443, url, true)) {  // HTTPS

      https.addHeader("Accept", "text/event-stream");

      if (!idToken.isEmpty()) {
        https.addHeader("Authorization", "Bearer " + idToken);
      }

      int httpCode = https.GET();

      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {

          while (https.connected()) {

            if (client->available()) {

              String line1 = client->readStringUntil('\n');

              if (line1.isEmpty()) continue;
              String line2 = client->readStringUntil('\n');

              if (line2.isEmpty()) continue;

              String event = line1.substring(line1.indexOf(":") + 2, line1.length());
              String body = line2.substring(line2.indexOf(":") + 2, line2.length());

              if (event == "auth_revoked" || event == "cancel") break;
              if (event != "put") continue;

              Serial.printf("\nFree Heap: %d, Heap Fragmentation: %d, Max Block Size: %d \n", ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize());

              String status = "";
              String state = "";

              DynamicJsonDocument* doc = JSON::parse(200, body);
              String path = (*doc)["path"].as<String>();
              if (path == "/") {
                status = (*doc)["data"]["status"].as<String>();
                state = (*doc)["data"]["state"].as<String>();
              } else if (path == "/state") {
                state = (*doc)["data"].as<String>();
              } else if (path == "/status") {
                status = (*doc)["data"].as<String>();
              }
              delete doc;

              if (state == "RESET") {
                response = new HttpResponse(HTTP_CODE_RESET_CONTENT, "");
                break;
              }

              if (!status.isEmpty()) {
                handler(status);
              }
            }
            delay(1);
          }

          Serial.println();
          Serial.print("[HTTPS] connection closed or file end.\n");
        } else {
          response = new HttpResponse(httpCode, https.getString());
        }
      } else {
        Serial.printf("[HTTPS] Fetch::GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Fetch::GET Unable to connect\n");
    }

    return response;
  }

private:
  static const char* getFingreprint(String host) {
    if (host == "metrix-3c2e5.firebaseio.com") {
      return RTDB_FINGERPRINT;
    }
    return FINGERPRINT;
  }
};