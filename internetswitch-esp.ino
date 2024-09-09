#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <memory>
#include "FS.h"
#include "Public.h"
#include "Firebase.h"

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

const char* HOSTNAME = "internetswitch"; //visit "internetswitch.local" to access the app

void setup() {
  Serial.begin(115200);
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  delay(3000);

  Serial.println(F("Disconnecting from current WiFi connection"));
  WiFi.disconnect();

  //Setup LED AS OUTPUT
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);


  //Initializing file system
  Serial.println(F("Mounting FS..."));
  if (!LittleFS.begin()) {
    Serial.println(F("Failed to mount file system"));
    return;
  }

  WiFi.mode(WIFI_AP_STA);


  if (isWiFiEnabled() && !isSetupPending()) {
    onSetupComplete();
    return;
  };

  Serial.println(F("Setup not ready, turning on HotSpot!"));

  //Start mDNS
  Serial.println(F("Starting MDNS..."));
  if (!MDNS.begin(HOSTNAME)) {
    Serial.println(F("Failed to start MDNS"));
    return;
  }

  setupAP();
  launchWeb();

  Serial.println(F("\nWaiting..."));

  while (isSetupPending()) {
    //Serial.printf("\nFree Heap: %d, Heap Fragmentation: %d, Max Block Size: %d", ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize());
    Serial.print(F("."));
    delay(100);
    server.handleClient();
    MDNS.update();
  }
  Serial.println(F("\nSetup Completed, Resetting ESP....."));

  delay(2000);

  ESP.reset();
}

void loop() {
  Serial.printf("\nFree Heap: %d, Heap Fragmentation: %d, Max Block Size: %d", ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize());
  delay(5000);
  if (isSetupPending()) {
    Serial.println(F("Setup pending..."));
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }

  Firebase::onStatusChangeRTDB([](String body) {
    Serial.printf("\nFree Heap: %d, Heap Fragmentation: %d, Max Block Size: %d", ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize());
    return processBody(body);
  });
}

int processBody(String body) {
  if (body.isEmpty()) return HTTP_CODE_OK;
  int index = body.indexOf('\n');

  String eventLine = body.substring(0, index);

  if (eventLine.isEmpty()) return HTTP_CODE_OK;

  String dataLine = body.substring(index + 1, body.indexOf('\n', index + 1));

  if (dataLine.isEmpty()) return HTTP_CODE_OK;

  String event = eventLine.substring(eventLine.indexOf(":") + 2, eventLine.length());
  String data = dataLine.substring(dataLine.indexOf(":") + 2, dataLine.length());

  if (event == "auth_revoked" || event == "cancel") return HTTP_CODE_FORBIDDEN;
  if (event == "keep-alive") return HTTP_CODE_OK;

  String status = "";
  String state = "";

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, data);
  if (error) {
    Serial.println(F("Failed to parse JSON"));
    Serial.println(error.c_str()); 
    return HTTP_CODE_OK;
  }
  String path = doc["path"].as<String>();

  if (path == "/") {
      status = doc["data"]["status"].as<String>();
      state = doc["data"]["state"].as<String>();
  } else if (path == "/state") {
      state = doc["data"].as<String>();
  } else if (path == "/status") {
      status = doc["data"].as<String>();
  }

  if (state == STATE_BREAK) {
    return HTTP_CODE_FORBIDDEN;
  }

  if (state == STATE_RESET) {
    return HTTP_CODE_RESET_CONTENT;
  }

  if (status.isEmpty()) return HTTP_CODE_OK;

  if (status == "null" && event == "put") {
    return HTTP_CODE_RESET_CONTENT;
  }

  if (status == "null" && event == "patch") {
    return HTTP_CODE_OK;
  }

  digitalWrite(LED_BUILTIN, status == STATUS_ON ? HIGH : LOW);
  Serial.printf("\nstatus: %s, event: %s", status, event);
  return HTTP_CODE_OK;
}


void onSetupComplete() {
  MDNS.end();
  server.stop();
  WiFi.mode(WIFI_STA);
  Serial.println(F("\nSetup ready, proceeding to firebase...."));
  Firebase::recordDeviceHistory("Device is setup successfully");
}

bool isSetupPending() {
  if (!WifiClient::isReady()) return true;
  if (WiFi.status() != WL_CONNECTED) return true;
  if (!Firebase::isReady()) return true;

  return false;
}

bool isWiFiEnabled() {
  //Read file for ssid and pass
  WiFiConfig config = WifiClient::getWifiConfig();
  if (!config.isValid()) {
    return false;
  }

  String ssid = config.getSSID();
  String password = config.getPassword();

  if (!WifiClient::testWifi(ssid, password)) {
    Serial.println(F("\n WiFi Not available!"));
    return false;
  }

  return true;
}

void setupAP(void) {
  Serial.println(F("Initializing hotspot...."));
  // WifiClient::disconnect();
  delay(100);
  WiFi.softAP("InternetSwitch", "");
  Serial.println(F("Hotspot Initialized!"));
}

void launchWeb() {
  Serial.println();
  delay(1000);
  if (WiFi.status() == WL_CONNECTED)
    Serial.println(F("WiFi connected"));
  Serial.print(F("Local IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("SoftAP IP: "));
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println(F("Server started"));
}

const char* html() {
  if (!WifiClient::isReady()) {
    return WIFI_HTML;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return WIFI_HTML;
  }
  if (!Firebase::isReady()) {
    return LOGIN_HTML;
  }
  return SUCCESS_HTML;
}

void createWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", html());
  });
  server.on("/styles", []() {
    server.send(200, "text/css", STYLES);
  });
  server.on("/script", []() {
    server.send(200, "text", SCRIPT);
  });
  server.on("/scan", []() {
    server.send(200, "application/json", WifiClient::scanWiFi());
  });
  server.on("/save", []() {
    //Check if body received
    if (server.hasArg("plain") == false) {
      server.send(400, "text/plain", "Body not received");
      return;
    }

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
      Serial.println(F("Failed to parse JSON"));
      Serial.println(error.c_str()); 
      server.send(400, "text/plain", "invalid request");
      return;
    }

    String ssid = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();


    if (!WifiClient::testWifi(ssid, password)) {
      server.send(400, "application/json", "Invalid ssid/password!");
      return;
    }

    if (!WifiClient::saveWifiConfig(ssid, password)) {
      server.send(500, "application/json", "Somer error has occured, Please try again later");
      return;
    }

    Serial.println(F("\nWiFi credentials saved successfully!"));

    server.send(200, "application/json", "Credentials Saved!");

    delay(2000);
  });
  server.on("/login", []() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Body not received");
        return;
    }

    WiFiConfig config = WifiClient::getWifiConfig();
    if (!config.isValid() || WiFi.status() != WL_CONNECTED) {
        server.send(400, "text/plain", "Wifi Not Available, Please Refresh!");
        return;
    }

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        Serial.print(F("Failed to parse JSON: "));
        Serial.println(error.c_str());
        server.send(400, "text/plain", "Invalid Body");
        return;
    }

    String email = doc["email"].as<String>();
    String password = doc["password"].as<String>();

    if (email.isEmpty() || password.isEmpty()) {
        server.send(400, "text/plain", "Invalid Credentials!");
        return;
    }

    HttpResponse* response = FirebaseAuth::signInWithEmailAndPassword(email, password);

    if (!response) {
        server.send(500, "text/plain", "Some error has occurred. Please try again later.");
        return;
    }

    auto responseBody = std::make_unique<DynamicJsonDocument>(2048);
    DeserializationError error1 = deserializeJson(*responseBody, response->getBody());
    int statusCode = response->getStatusCode();
    delete response;  // Ensure this is appropriate for your response management

    if (error1) {
        Serial.print(F("Failed to parse JSON: "));
        Serial.println(error1.c_str());
        server.send(500, "text/plain", "Invalid response from firebase");
        return;
    }

    handleLogin(statusCode, std::move(responseBody));
  });
}

void handleLogin(int statusCode, std::unique_ptr<DynamicJsonDocument> body) {
    if (!body) {
        server.send(500, "text/plain", "Invalid body received");
        return;
    }

    if (statusCode == 200) {
        Serial.println(F("\nSignin successful"));

        String localId = (*body)["localId"].as<String>();
        String idToken = (*body)["idToken"].as<String>();
        String refreshToken = (*body)["refreshToken"].as<String>();

        Serial.println(F("\nCreating device..."));

        String deviceId = Firebase::createDevice(localId, idToken);
        if (deviceId.isEmpty()) {
            server.send(500, "text/plain", "Some error has occurred. Please try again later.");
            return;
        }

        Serial.println(F("Device created successfully!"));

        if (!Firebase::saveFirebaseConfig(localId, idToken, refreshToken, deviceId)) {
            Firebase::deleteDeviceFromRTDB(localId, deviceId, idToken);
            server.send(500, "text/plain", "Some error has occurred. Please try again later.");
            return;
        }

        Serial.println(F("\nFirebase configurations saved successfully!"));

        Serial.println(F("\nInforming Client"));

        server.send(200, "text/plain", "Device Registered <span style='color: var(--primary);'>Successfully</span></span>");

        Serial.println(F("\nClient informed successfully!"));

        Serial.println(F("\nResetting ESP..."));
        delay(5000);
        ESP.reset();
    } else if (statusCode == 400) {
        if (!body->containsKey("error")) {
            server.send(500, "text/plain", "Some error has occurred. Please try again later.");
            return;
        }

        String errorCode = (*body)["error"]["message"].as<String>();
        String message = "BAD_REQUEST";

        if (errorCode == "EMAIL_NOT_FOUND") {
            message = "Please signup & register to continue";
        } else if (errorCode == "INVALID_PASSWORD") {
            message = "Please enter a valid email/password";
        } else if (errorCode == "USER_DISABLED") {
            message = "Your account is disabled, Please reach out to support.";
        }

        server.send(400, "text/plain", message);
    } else {
        server.send(500, "text/plain", "Some error has occurred. Please try again later.");
    }
}

