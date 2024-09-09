#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "FirebaseConstants.h"
#include "HttpResponse.h"



typedef std::function<int(String)> StreamHandler;

class Fetch {

public:
  static bool DELETE(String url) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    bool result = false;

    if (https.begin(*client, url)) {

      int httpCode = https.DELETE();

      if (httpCode > 0) {
        Serial.printf("\n[HTTPS] Fetch::DELETE httpCode: %d, url: %s\n", httpCode, url.c_str());
        if (httpCode == HTTP_CODE_OK) {
          result = true;
        } else {
        }
      } else {
        Serial.printf("\n[HTTPS] Fetch::DELETE failed, httpCode: %d, url: %s\n", httpCode, url.c_str());
      }
      https.end();
    } else {
      Serial.printf("\n[HTTPS] deleteDocument Unable to connect url: %s\n", url.c_str());
    }

    return result;
  }


  static HttpResponse* POST(String url, String payload) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setInsecure();
    HTTPClient https;

    HttpResponse* response = nullptr;
    if (https.begin(*client, url)) {
      int httpCode = https.POST(payload);
      if (httpCode > 0) {
        Serial.printf("\n[HTTPS] Fetch::POST httpCode: %d, url: %s\n", httpCode, url.c_str());
        String payload = https.getString();
        response = new HttpResponse(httpCode, payload);
      } else {
        Serial.printf("\n[HTTPS] Fetch::POST failed, httpCode: %d, url: %s\n", httpCode, url.c_str());
      }
      https.end();
    } else {
      Serial.println(F("\n[HTTPS]  Fetch::POST  Unable to connect\n"));
    }
    return response;
  }

  static HttpResponse* GET(String url) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setInsecure();

    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, url)) {

      int httpCode = https.GET();

      if (httpCode > 0) {
        Serial.printf("\n[HTTPS] Fetch::GET httpCode: %d, url: %s\n", httpCode, url.c_str());
        String payload = https.getString();
        response = new HttpResponse(httpCode, payload);
      } else {
        Serial.printf("\n[HTTPS] Fetch::GET failed, httpCode: %d, url: %s\n", httpCode, url.c_str());
      }
      https.end();
    } else {
      Serial.println(F("\n[HTTPS] Fetch::GET Unable to connect\n"));
    }

    return response;
  }

  static HttpResponse* ON(String url, StreamHandler handler) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setInsecure();

    HTTPClient https;

    HttpResponse* response = nullptr;

    if (https.begin(*client, url)) {

      https.addHeader("Accept", "text/event-stream");

      int httpCode = https.GET();

      if (httpCode > 0) {
        Serial.printf("\n[HTTPS] Fetch::ON httpCode: %d, url: %s\n", httpCode, url.c_str());
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
          Serial.println(F("\n[HTTPS] Fetch::ON connection closed or file end.\n"));
        }
        response = new HttpResponse(httpCode, https.getString());
      } else {
        Serial.printf("\n[HTTPS] Fetch::ON failed, httpCode: %d, url: %s\n", httpCode, url.c_str());
      }
      https.end();
    } else {
      Serial.printf("\n[HTTPS] Fetch::ON Unable to connect, url: %s\n", url.c_str());
    }

    return response;
  }
};