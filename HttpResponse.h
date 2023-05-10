#include "JSON.h"

class HttpResponse {
private:
  int statusCode;
  String body;

public:
  HttpResponse(int httpStatus, String responseBody) {
    statusCode = httpStatus;
    body = responseBody;
  }
  int getStatusCode() {
    return statusCode;
  }
  String getBody() {
    return body;
  }
  DynamicJsonDocument* json(int size) {
    return JSON::parse(size, body);
  }
  DynamicJsonDocument* json() {
    return JSON::parse(body);
  }
};
