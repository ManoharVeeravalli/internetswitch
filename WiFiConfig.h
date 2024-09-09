
class WiFiConfig : public IsValid {
private:
  String ssid_;
  String password_;
public:
  WiFiConfig(String ssid, String password) {
    ssid_ = ssid;
    password_ = password;
  }
  WiFiConfig() {
    ssid_ = "";
    password_ = "";
  }
  String getSSID() {
    return ssid_;
  }
  String getPassword() {
    return password_;
  }

  bool isValid() const override {
    if (ssid_.isEmpty() || password_.isEmpty()) {
      return false;
    }

    return true;
  }
};