#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "FirebaseConstants.h"
#include "HttpResponse.h"



typedef std::function<int(String)> StreamHandler;

class Fetch {

public:
  static bool DELETE(String host, String url, String idToken) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setFingerprint(getFingreprint(host));
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
          Serial.printf("\n[HTTPS] Fetch::DELETE DELETE... failed, error: %s, url: %s\n", https.errorToString(httpCode).c_str(), url.c_str());
        }
      } else {
        Serial.printf("\n[HTTPS] Fetch::DELETE DELETE... failed, error: %s, url: %s\n", https.errorToString(httpCode).c_str(), url.c_str());
      }

      https.end();
    } else {
      Serial.printf("\n[HTTPS] deleteDocument Unable to connect url: %s\n", url.c_str());
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
        Serial.printf("\n[HTTPS] Fetch::POST POST... failed, error: %s, url: %s\n", https.errorToString(httpCode).c_str(), url.c_str());
      }

      https.end();
    } else {
      Serial.printf("\n[HTTPS]  Fetch::POST  Unable to connect\n");
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
        Serial.printf("\n[HTTPS] Fetch::GET GET... failed, error: %s, url: %s\n", https.errorToString(httpCode).c_str(), url.c_str());
      }
      https.end();
    } else {
      Serial.printf("\n[HTTPS] Fetch::GET Unable to connect\n");
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
              httpCode = handler(body);
              if (httpCode != HTTP_CODE_OK) {
                break;
              };

              if (len > 0) { len -= c; }
            }
            delay(1);
          }
          Serial.println();
          Serial.print("\n[HTTPS] Fetch::ON connection closed or file end.\n");
          response = new HttpResponse(httpCode, "");
        } else {
          Serial.printf("\n[HTTPS] Fetch::ON... failed, error: %s, url: %s\n", https.errorToString(httpCode).c_str(), url.c_str());
          response = new HttpResponse(httpCode, https.getString());
        }
      } else {
        Serial.printf("\n[HTTPS] Fetch::ON... failed, error: %s, url: %s\n", https.errorToString(httpCode).c_str(), url.c_str());
      }
      https.end();
    } else {
      Serial.printf("\n[HTTPS] Fetch::ON Unable to connect, url: %s\n", url.c_str());
    }

    return response;
  }

private:
  static const char* getFingreprint(String host) {
    if (host == "internetswitch-d4d02-default-rtdb.firebaseio.com") {
      Serial.println("\nRTDB_FINGERPRINT");
      return RTDB_FINGERPRINT;
    }
    Serial.println("\nFINGERPRINT");
    return FINGERPRINT;
  }
};