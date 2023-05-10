#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include "FS.h"
#include "Public.h"
#include "Firebase.h"
#include "WiFiClient.h"


//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

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

  //only for dev
  // LittleFS.remove(WIFI_CONFIG_FILE);
  // LittleFS.remove(FIREBASE_CONFIG_FILE);

  WiFi.mode(WIFI_AP_STA);


  if (isWiFiEnabled() && Firebase::isReady()) {
    onSetupComplete();
    return;
  };

  Serial.println("Setup not ready, turning the HotSpot On");

  setupAP();
  launchWeb();


  Serial.println("\nWaiting...");

  while (isSetupPending()) {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }

  onSetupComplete();
}

void loop() {
  delay(5000);
  Serial.printf("\nFree Heap: %d, Heap Fragmentation: %d, Max Block Size: %d \n", ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize());
  if (isSetupPending()) {
    return;
  }

  String status = Firebase::getStatusFromFirebase();

  if (status.isEmpty()) return;

  digitalWrite(LED_BUILTIN, status == "HIGH" ? HIGH : LOW);
  Serial.println(status);
}


void onSetupComplete() {
  WiFi.mode(WIFI_STA);
  Serial.println("\nSetup ready, proceeding to firebase....");
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
  WifiClient::disconnect();
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

void createWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", WifiClient::isReady() ? LOGIN_HTML : INDEX_HTML);
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
      server.send(400, "text/plain", "Some error has occured, please refresh and try again!");
      return;
    }
    String ssid = config->getSSID();
    String pass = config->getPassword();

    delete config;

    if (!WifiClient::testWifi(ssid, pass)) {
      server.send(500, "application/json", "Some error has occured, please refresh and try again!");
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

    if (!(WiFi.status() == WL_CONNECTED)) {
      server.send(400, "text/plain", "Wifi Not Available!");
      return;
    }

    HttpResponse* response = Firebase::signInWithEmailAndPassword(email, password);

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

    Serial.println("\nDisconnecting from WiFi...");
    WifiClient::disconnect();

    if (!WifiClient::saveWifiConfig(ssid, password)) {
      server.send(500, "application/json", "Somer error has occured, Please try again later");
      return;
    }

    server.send(200, "application/json", "Credentials Saved!");
  });
}

void handleLogin(int statusCode, DynamicJsonDocument* body) {
  if (statusCode == 200) {
    String localId = (*body)["localId"].as<String>();
    String idToken = (*body)["idToken"].as<String>();
    String refreshToken = (*body)["refreshToken"].as<String>();

    HttpResponse* resp = Firebase::createDeviceDocument(localId, idToken);
    if (!resp) {
      server.send(400, "text/plain", "Somer error has occured, Please try again later");
      return;
    }
    int statusCode = resp->getStatusCode();
    DynamicJsonDocument* doc = resp->json(384);

    delete resp;

    if (!doc) {
      server.send(400, "text/plain", "Somer error has occured, Please try again later");
      return;
    }

    String name = (*doc)["name"].as<String>();

    String deviceId = Firebase::getDeviceIDFromName(name);

    if (!Firebase::saveFirebaseConfig(localId, idToken, refreshToken, deviceId)) {
      server.send(500, "text/plain", "Somer error has occured, Please try again later");
      return;
    }
    server.send(200, "text/plain", "Device registred successfully!");

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
    message = "Please signup using app to continue";
  }
  if (errorCode == "INVALID_PASSWORD") {
    message = "Please enter valid email/password";
  }
  if (errorCode == "USER_DISABLED") {
    message = "You are disabled, Please reach out to support";
  }

  server.send(400, "text/plain", message);
}
