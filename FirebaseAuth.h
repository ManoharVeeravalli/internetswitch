#include "Fetch.h"
#include "StringUtils.h"
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

    return Fetch::POST(FIRESTORE_HOST, LOGIN_URL, payload, "");
  }

  static FirebaseConfig* regenerateToken(String refreshToken) {
    DynamicJsonDocument payload(384);
    payload["grant_type"] = "refresh_token";
    payload["refresh_token"] = refreshToken;

    FirebaseConfig* config = nullptr;

    HttpResponse* response = Fetch::POST(FIRESTORE_HOST, REFRESH_TOKEN_URL, JSON::stringify(payload), "");

    if(!response) {
      Serial.println("\ninvalid response");
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

    String newLocalId = (*doc)["user_id"].as<String>();
    String newIdToken = (*doc)["id_token"].as<String>();
    String newRefreshToken = (*doc)["refresh_token"].as<String>();
    String deviceId = "";

    delete doc;

    config = new FirebaseConfig(newLocalId, newIdToken, newRefreshToken, deviceId);

    return config;
  }
};