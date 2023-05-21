#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "HttpResponse.h"

const char* FINGERPRINT = "A7 7B 0F F6 B0 8B 9B CA A7 0B 1A 82 76 10 B2 64 10 BB 17 0A";

const char* RTDB_FINGERPRINT = "91 14 41 84 C3 F8 48 9D 29 56 8C D4 35 43 F6 B8 53 F1 FE FE";

typedef std::function<bool(String)> StreamHandler;

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

          // get length of document (is -1 when Server sends no Content-Length header)
          int len = https.getSize();

          // create buffer for read
          static uint8_t buff[128] = { 0 };

          // read all data from server
          while (https.connected() && (len > 0 || len == -1)) {
            // get available data size
            size_t size = client->available();

            if (size) {
              // read up to 128 byte
              int c = client->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

              String body = "";

              for (int i = 0; i < c; i++) {
                body += ((char)buff[i]);
              }

              // write it to handler
              if (handler(body)) {
                break;
              };

              if (len > 0) { len -= c; }
            }
            delay(1);
          }
          Serial.println();
          Serial.print("[HTTPS] Fetch::ON connection closed or file end.\n");
          response = new HttpResponse(httpCode, "");
        } else {
          response = new HttpResponse(httpCode, https.getString());
        }
      } else {
        Serial.printf("[HTTPS] Fetch::ON... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Fetch::ON Unable to connect\n");
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