#include "FirebaseAuth.h"


class Firestore {
public:
  static bool deleteDocument(String path, String idToken) {
    return Fetch::DELETE(FIRESTORE_HOST, FIRESTORE_BASE_URL + path, idToken);
  }

  static HttpResponse* createDocument(String path, String payload, String idToken) {
    return Fetch::POST(FIRESTORE_HOST, FIRESTORE_BASE_URL + path, payload, idToken);
  }

  static HttpResponse* getDocument(String path, String idToken) {
    return Fetch::GET(FIRESTORE_HOST, FIRESTORE_BASE_URL + path, idToken);
  }
};