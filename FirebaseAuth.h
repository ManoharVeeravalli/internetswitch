#include "Fetch.h"
#include "StringUtils.h"
#include "FirebaseConfig.h"


class FirebaseAuth {

public:

  static HttpResponse* signInWithEmailAndPassword(String email, String password) {
    StaticJsonDocument<200> doc;
    doc["email"] = email;
    doc["password"] = password;
    doc["returnSecureToken"] = true;

    String payload = "";
    serializeJson(doc, payload);

    return Fetch::POST(LOGIN_URL, payload);
  }

  static FirebaseConfig regenerateToken(String refreshToken) {
    StaticJsonDocument<384> payload;
    payload["grant_type"] = "refresh_token";
    payload["refresh_token"] = refreshToken;

    String requestBody = "";
    serializeJson(payload, requestBody);

    FirebaseConfig config;

    HttpResponse* response = Fetch::POST(REFRESH_TOKEN_URL, requestBody);

    if(!response) {
      Serial.println(F("\ninvalid response"));
      return config;
    }

    int httpCode = response->getStatusCode();
    String body = response->getBody();

    delete response;

    if (httpCode != HTTP_CODE_OK) {
      Serial.printf("\nInvalid statusCode %d", httpCode);
      return config;
    }

    auto doc = std::make_unique<DynamicJsonDocument>(3072);
    DeserializationError error = deserializeJson(*doc, body);

    if (error) {
        Serial.print(F("Failed to parse JSON: "));
        Serial.println(error.c_str());
        return config;
    }


    String newLocalId = (*doc)[LOCAL_ID_REGENERATE].as<String>();
    String newIdToken = (*doc)[ID_TOKEN_REGENERATE].as<String>();
    String newRefreshToken = (*doc)[REFRESH_TOKEN_REGENERATE].as<String>();
    String deviceId = "";

    config = FirebaseConfig(newLocalId, newIdToken, newRefreshToken, deviceId);

    return config;
  }
};