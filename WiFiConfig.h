

class WiFiConfig {
private:
  String ssid;
  String password;
public:
  WiFiConfig(String id, String pass) {
    ssid = id;
    password = pass;
  }
  String getSSID() {
    return ssid;
  }
  String getPassword() {
    return password;
  }
};