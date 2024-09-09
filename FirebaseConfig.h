

class FirebaseConfig : public IsValid {
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
  FirebaseConfig() {
    localId_ = "";
    idToken_ = "";
    refreshToken_ = "";
    deciveId_ = "";
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

  bool isValid() const override {
    if (localId_.isEmpty() || idToken_.isEmpty() || refreshToken_.isEmpty()) {
      return false;
    }
    return true;
  }
};