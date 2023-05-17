

class WiFiConfig {
private:
  String ssid_;
  String password_;
public:
  WiFiConfig(String ssid, String password) {
    ssid_ = ssid;
    password_ = password;
  }
  String getSSID() {
    return ssid_;
  }
  String getPassword() {
    return password_;
  }
};