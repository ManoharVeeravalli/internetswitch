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


  static HttpResponse* PATCH(String url, String payload) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setInsecure();
    HTTPClient https;

    HttpResponse* response = nullptr;
    if (https.begin(*client, url)) {
      int httpCode = https.PATCH(payload);
      if (httpCode > 0) {
        Serial.printf("\n[HTTPS] Fetch::PATCH httpCode: %d, url: %s\n", httpCode, url.c_str());
        String payload = https.getString();
        response = new HttpResponse(httpCode, payload);
      } else {
        Serial.printf("\n[HTTPS] Fetch::PATCH failed, httpCode: %d, url: %s\n", httpCode, url.c_str());
      }
      https.end();
    } else {
      Serial.println(F("\n[HTTPS]  Fetch::PATCH  Unable to connect\n"));
    }
    return response;
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

static std::unique_ptr<HttpResponse> ON(String url,  unsigned long ttl, StreamHandler handler) {
  // Start the time tracker for the loop
  unsigned long startTime = millis();

  // Use a unique_ptr to handle the BearSSL::WiFiClientSecure to avoid manual delete
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

  client->setInsecure();
  HTTPClient https;

  std::unique_ptr<HttpResponse> response = nullptr;  // Response is now a unique_ptr

  if (https.begin(*client, url)) {
    https.addHeader("Accept", "text/event-stream");

    int httpCode = https.GET();
    if (httpCode > 0) {
      Serial.printf("\n[HTTPS] Fetch::ON httpCode: %d, url: %s\n", httpCode, url.c_str());

      if (httpCode == HTTP_CODE_OK) {
        // Get the length of the document (is -1 when Server sends no Content-Length header)
        int len = https.getSize();
        static uint8_t buff[128] = { 0 };

        // Read all data from the server with timeout check based on provided ttl
        while (https.connected() && (len > 0 || len == -1)) {
          // Check if the elapsed time since start exceeds ttl
          if (millis() - startTime >= ttl) {
            Serial.println(F("\n[HTTPS] Fetch::ON exiting loop due to TTL.\n"));
            break;
          }

          // Get available data size
          size_t size = client->available();
          if (size) {
            // Read up to 128 bytes
            int c = client->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

            String body = "";
            for (int i = 0; i < c; i++) {
              body += static_cast<char>(buff[i]);
            }

            // Pass body data to the handler
            httpCode = handler(body);
            if (httpCode != HTTP_CODE_OK) {
              break;
            }

            if (len > 0) {
              len -= c;
            }
          }
          delay(1);
        }
        Serial.println(F("\n[HTTPS] Fetch::ON connection closed or file end.\n"));
      }

      // Use unique_ptr to manage the response memory
      response = std::make_unique<HttpResponse>(httpCode, https.getString());
    } else {
      Serial.printf("\n[HTTPS] Fetch::ON failed, httpCode: %d, url: %s\n", httpCode, url.c_str());
    }

    // Always close the connection properly
    https.end();
  } else {
    Serial.printf("\n[HTTPS] Fetch::ON Unable to connect, url: %s\n", url.c_str());
  }

  // Return the response as a unique_ptr to ensure proper memory handling
  return response;
}

};