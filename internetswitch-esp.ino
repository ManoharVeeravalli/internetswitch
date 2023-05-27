#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include "FS.h"
#include "Public.h"
#include "Firebase.h"

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

const char* HOSTNAME = "internetswitch";

void setup() {
  Serial.begin(115200);
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  delay(3000);

  Serial.println("Disconnecting from current WiFi connection");
  WiFi.disconnect();

  //Setup LED AS OUTPUT
  pinMode(LED_BUILTIN, OUTPUT);


  //Initializing file system
  Serial.println("Mounting FS...");
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  //only dev
  // LittleFS.remove(WIFI_CONFIG_FILE);
  // LittleFS.remove(FIREBASE_CONFIG_FILE);


  WiFi.mode(WIFI_AP_STA);


  if (isWiFiEnabled() && !isSetupPending()) {
    onSetupComplete();
    return;
  };

  Serial.println("Setup not ready, turning on HotSpot!");

  //Start mDNS
  Serial.println("Starting MDNS...");
  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("Failed to start MDNS");
    return;
  }

  setupAP();
  launchWeb();

  Serial.println("\nWaiting...");

  while (isSetupPending()) {
    Serial.printf("\nFree Heap: %d, Heap Fragmentation: %d, Max Block Size: %d", ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize());
    Serial.print(".");
    delay(100);
    server.handleClient();
    MDNS.update();
  }

  onSetupComplete();
}

void loop() {
  Serial.printf("\nFree Heap: %d, Heap Fragmentation: %d, Max Block Size: %d", ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize());
  delay(5000);
  if (isSetupPending()) {
    Serial.println("Setup pending...");
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

  DynamicJsonDocument* doc = JSON::parse(200, data);
  String path = (*doc)["path"].as<String>();
  if (path == "/") {
    status = (*doc)["data"]["status"].as<String>();
    state = (*doc)["data"]["state"].as<String>();
  } else if (path == "/state") {
    state = (*doc)["data"].as<String>();
  } else if (path == "/status") {
    status = (*doc)["data"].as<String>();
  }
  delete doc;



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

  digitalWrite(LED_BUILTIN, status == STATUS_ON ? HIGH : LOW);
  Serial.printf("\nstatus: %s", status);
  return HTTP_CODE_OK;
}


void onSetupComplete() {
  MDNS.end();
  server.stop();
  WiFi.mode(WIFI_STA);
  Serial.println("\nSetup ready, proceeding to firebase....");
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
  WiFiConfig* config = WifiClient::getWifiConfig();
  if (!config) {
    return false;
  }

  String ssid = config->getSSID();
  String password = config->getPassword();

  delete config;

  if (!WifiClient::testWifi(ssid, password)) {
    Serial.println("\n WiFi Not available!");
    return false;
  }

  return true;
}

void setupAP(void) {
  Serial.println("Initializing hotspot....");
  // WifiClient::disconnect();
  delay(100);
  WiFi.softAP("InternetSwitch", "");
  Serial.println("Hotspot Initialized!");
}

void launchWeb() {
  Serial.println("");
  delay(1000);
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
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
  server.on("/login", []() {
    if (server.hasArg("plain") == false) {
      server.send(400, "text/plain", "Body not received");
      return;
    }
    WiFiConfig* config = WifiClient::getWifiConfig();
    if (!config) {
      server.send(400, "text/plain", "Wifi Not Available, Please Refresh!");
      return;
    }
    delete config;


    // String ssid = config->getSSID();
    // String pass = config->getPassword();

    // delete config;

    // if (!WifiClient::testWifi(ssid, pass)) {
    //   server.send(500, "application/json", "Some error has occured, please refresh and try again!");
    //   return;
    // }


    if (!(WiFi.status() == WL_CONNECTED)) {
      server.send(400, "text/plain", "Wifi Not Available, Please Refresh!");
      return;
    }

    DynamicJsonDocument* doc = JSON::parse(server.arg("plain"));
    if (!doc) {
      server.send(400, "text/plain", "Invalid Body");
      return;
    }
    String email = (*doc)["email"].as<String>();
    String password = (*doc)["password"].as<String>();

    delete doc;

    if (email == "") {
      server.send(400, "text/plain", "Invalid Credentials!");
      return;
    }

    if (password == "") {
      server.send(400, "text/plain", "Invalid Credentials!");
      return;
    }


    HttpResponse* response = FirebaseAuth::signInWithEmailAndPassword(email, password);

    if (!response) {
      server.send(500, "text/plain", "Somer error has occured, Please try again later");
      return;
    }

    DynamicJsonDocument* body = response->json(2048);

    int statusCode = response->getStatusCode();

    delete response;

    if (!body) {
      server.send(500, "text/plain", "Invalid response from firebase");
      return;
    }

    handleLogin(statusCode, body);

    delete body;

    server.send(200, "text/plain", "Success!");
  });
  server.on("/save", []() {
    //Check if body received
    if (server.hasArg("plain") == false) {
      server.send(400, "text/plain", "Body not received");
      return;
    }
    DynamicJsonDocument* doc = JSON::parse(server.arg("plain"));
    if (!doc) {
      server.send(400, "text/plain", "invalid request");
      return;
    }

    String ssid = (*doc)["ssid"].as<String>();
    String password = (*doc)["password"].as<String>();

    delete doc;

    if (!WifiClient::testWifi(ssid, password)) {
      server.send(400, "application/json", "Invalid ssid/password!");
      return;
    }

    if (!WifiClient::saveWifiConfig(ssid, password)) {
      server.send(500, "application/json", "Somer error has occured, Please try again later");
      return;
    }

    Serial.println("\nWiFi credentials saved successfully!");

    server.send(200, "application/json", "Credentials Saved!");
  });
}

void handleLogin(int statusCode, DynamicJsonDocument* body) {
  if (statusCode == 200) {
    Serial.println("\nsignin successful");
    String localId = (*body)[LOCAL_ID].as<String>();
    String idToken = (*body)[ID_TOKEN].as<String>();
    String refreshToken = (*body)[REFRESH_TOKEN].as<String>();

    delete body;

    Serial.println("\ncreating device....");


    String deviceId = Firebase::createDevice(localId, idToken);


    if (deviceId.isEmpty()) {
      server.send(500, "text/plain", "Somer error has occured, Please try again later");
      return;
    }

    Serial.println("device created successfully!");


    if (!Firebase::saveFirebaseConfig(localId, idToken, refreshToken, deviceId)) {
      Firebase::deleteDeviceFromRTDB(localId, deviceId, idToken);
      server.send(500, "text/plain", "Somer error has occured, Please try again later");
      return;
    }

    Serial.println("\nFirebase configurations saved successfully!");

    server.send(200, "text/plain", "Device Registered <span style='color: var(--primary);'>Successfully</span></span>");

    delay(2000);


    ESP.reset();
  }

  if (statusCode != 400) {
    server.send(500, "text/plain", "Somer error has occured, Please try again later");
    return;
  }

  if (!(body->containsKey("error"))) {
    server.send(500, "text/plain", "Somer error has occured, Please try again later");
    return;
  }

  String errorCode = (*body)["error"]["message"].as<String>();

  String message = "BAD_REQUEST";

  if (errorCode == "EMAIL_NOT_FOUND") {
    message = "Please signup & register to continue";
  }
  if (errorCode == "INVALID_PASSWORD") {
    message = "Please enter valid email/password";
  }
  if (errorCode == "USER_DISABLED") {
    message = "You are disabled, Please reach out to support";
  }

  server.send(400, "text/plain", message);
}
