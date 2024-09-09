#include "Fetch.h"
#include "StringUtils.h"
#include "FirebaseConfig.h"


class FirebaseAuth {

public:

  static HttpResponse* signInWithEmailAndPassword(String email, String password) {
    DynamicJsonDocument doc(200);
    doc["email"] = email;
    doc["password"] = password;
    doc["returnSecureToken"] = true;

    String payload = JSON::stringify(doc);

    return Fetch::POST(LOGIN_URL, payload);
  }

  static FirebaseConfig* regenerateToken(String refreshToken) {
    DynamicJsonDocument payload(384);
    payload["grant_type"] = "refresh_token";
    payload["refresh_token"] = refreshToken;

    FirebaseConfig* config = nullptr;

    HttpResponse* response = Fetch::POST(REFRESH_TOKEN_URL, JSON::stringify(payload));

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

    DynamicJsonDocument* doc = JSON::parse(3072, body);

    if (!doc) {
      return config;
    }

    String newLocalId = (*doc)[LOCAL_ID_REGENERATE].as<String>();
    String newIdToken = (*doc)[ID_TOKEN_REGENERATE].as<String>();
    String newRefreshToken = (*doc)[REFRESH_TOKEN_REGENERATE].as<String>();
    String deviceId = "";

    delete doc;

    config = new FirebaseConfig(newLocalId, newIdToken, newRefreshToken, deviceId);

    return config;
  }
};