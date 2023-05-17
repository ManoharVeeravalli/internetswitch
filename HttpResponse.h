#include "JSON.h"

class HttpResponse {
private:
  int statusCode_;
  String body_;

public:
  HttpResponse(int statusCode, String body) {
    statusCode_ = statusCode;
    body_ = body;
  }
  int getStatusCode() {
    return statusCode_;
  }
  String getBody() {
    return body_;
  }
  DynamicJsonDocument* json(int size) {
    return JSON::parse(size, body_);
  }
  DynamicJsonDocument* json() {
    return JSON::parse(body_);
  }
};
