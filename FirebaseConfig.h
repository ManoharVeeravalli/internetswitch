

class FirebaseConfig {
private:
  String localId_;
  String idToken_;
  String refreshToken_;
  String deciveId_;
public:
  FirebaseConfig(String& localId, String& idToken, String& refreshToken, String& deciveId) {
    localId_ = localId;
    idToken_ = idToken;
    refreshToken_ = refreshToken;
    deciveId_ = deciveId;
  }
  String getLocalID() {
    return localId_;
  }
  String getToken() {
    return idToken_;
  }
  String getRefreshToken() {
    return refreshToken_;
  }
  String getDeviceID() {
    return deciveId_;
  }
};