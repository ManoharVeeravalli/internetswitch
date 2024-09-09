#include "JSON.h"

class HttpResponse : public IsValid {
private:
  int statusCode_;
  String body_;

public:
  HttpResponse() {
    statusCode_ = 0;
    body_ = "";
  }

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
  bool isValid() const override {
    return statusCode_ != 0;
  }
};
