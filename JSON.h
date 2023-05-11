#include <ArduinoJson.h>

class JSON {
public:
  static String stringify(JsonObject& obj) {
    String data = "";
    serializeJson(obj, data);
    return data;
  }
  static String stringify(DynamicJsonDocument doc) {
    String data = "";
    serializeJson(doc, data);
    return data;
  }
  static void stringify(DynamicJsonDocument doc, File file) {
    serializeJson(doc, file);
  }
  static DynamicJsonDocument* parse(int size, String str) {
    DynamicJsonDocument* doc = new DynamicJsonDocument(size);
    DeserializationError error = deserializeJson(*doc, str);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      delete doc;
      return nullptr;
    }
    return doc;
  }
    static DynamicJsonDocument* parse(int size, File file) {
    DynamicJsonDocument* doc = new DynamicJsonDocument(size);
    DeserializationError error = deserializeJson(*doc, file);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      delete doc;
      return nullptr;
    }
    return doc;
  }
  static DynamicJsonDocument* parse(String str) {
    DynamicJsonDocument* doc = new DynamicJsonDocument(200);
    DeserializationError error = deserializeJson(*doc, str);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      delete doc;
      return nullptr;
    }
    return doc;
  }
};