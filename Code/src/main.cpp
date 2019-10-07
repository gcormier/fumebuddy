#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#define GET_CHIPID()  ((uint16_t)(ESP.getEfuseMac()>>32))
#include <FS.h>
#include <AutoConnect.h>
#include <ArduinoOTA.h>

// AutoConnect stuff
#define PARAM_FILE      "/param.json"
#define AUX_ESPURNASETTING "/espurna_setting"
#define AUX_ESPURNASAVE    "/espurna_save"

// Fumebuddy Definitions
#define PIN_SENSOR      17
#define PIN_RELAY_AUX   14
#define PIN_RELAY_NC    5
#define PIN_RELAY_NO    12
#define PIN_BUZZER      26
#define OFFHOOK         HIGH
#define ONHOOK          LOW
#define ONHOOK_DELAY     5L   // Delay before turning off in seconds

enum class HookState {
  STATE_OFFHOOK,      // Off hook
  STATE_ONHOOK_READY, // Ready for On Hook
  STATE_ONHOOK        // On hook
};
bool hookState = ONHOOK;
uint32_t currentMillis = 0;
uint32_t onHookMillis = 0;
HookState currentState;

typedef WebServer WiFiWebServer;

AutoConnect       portal;
AutoConnectConfig config;
WiFiClient        wifiClient;

String espurnaServer;
String espurnaApiKey;

HTTPClient http;

bool readHookState()
{
  return digitalRead(PIN_SENSOR);
}



void startDevice()
{
  http.begin("http://" + espurnaServer + "/api/relay/0?apikey=" + espurnaApiKey + "&value=1");
  http.GET();
  http.end();
}

void stopDevice()
{
  http.begin("http://" + espurnaServer + "/api/relay/0?apikey=" + espurnaApiKey + "&value=0");
  http.GET();
  http.end();
}

void toggleDevice()
{
  http.begin("http://" + espurnaServer + "/api/relay/0?apikey=" + espurnaApiKey + "&value=2");
  http.GET();
  http.end();
}


void offHook()
{
  if (hookState == ONHOOK)
  {
    digitalWrite(PIN_RELAY_NO, LOW);
    hookState = OFFHOOK;
    startDevice();
    onHookMillis = 0;
    Serial.println("Going off hook");
  }
}

void onHook()
{
  if (hookState == OFFHOOK)
  {
    digitalWrite(PIN_RELAY_NO, HIGH);
    hookState = ONHOOK;
    //stopDevice();
    onHookMillis = millis() + (ONHOOK_DELAY * 1000L);
    Serial.println("Going on hook");
  }
}

String loadParams(AutoConnectAux& aux, PageArgument& args) {
  (void)(args);
  File param = SPIFFS.open(PARAM_FILE, "r");
  if (param) {
    aux.loadElement(param);
    param.close();
  }
  else
    Serial.println(PARAM_FILE " open failed");
  return String("");
}

String saveParams(AutoConnectAux& aux, PageArgument& args) {
  
  espurnaServer = args.arg("espurnaserver");
  espurnaServer.trim();

  espurnaApiKey = args.arg("espurnaapikey");
  espurnaApiKey.trim();

  File param = SPIFFS.open(PARAM_FILE, "w");
  portal.aux("/espurna_setting")->saveElement(param, { "espurnaserver", "espurnaapikey" });
  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText&  echo = aux["parameters"].as<AutoConnectText>();
  echo.value += "espurna host: " + espurnaServer + "<br>";
  echo.value += "ESP api key: " + espurnaApiKey + "<br>";

  return String("");
}

void handleRoot() {
  String  content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "</head>"
    "<body>"
    "<p style=\"padding-top:10px;text-align:left\"> Settings: " AUTOCONNECT_LINK(COG_24) "</p>"
    "</body>"
    "</html>";

  WiFiWebServer&  webServer = portal.host();
  webServer.send(200, "text/html", content);
}

// Load AutoConnectAux JSON from SPIFFS.
bool loadAux(const String auxName) {
  bool  rc = false;
  String  fn = auxName + ".json";
  File fs = SPIFFS.open(fn.c_str(), "r");
  if (fs) {
    rc = portal.load(fs);
    fs.close();
  }
  else
    Serial.println("SPIFFS open failed: " + fn);
  return rc;
}

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println();
  SPIFFS.begin();

  // Fumebuddy setup
  pinMode(PIN_RELAY_NO, OUTPUT);
  pinMode(PIN_RELAY_NC, OUTPUT);
  pinMode(PIN_SENSOR, INPUT_PULLUP);
  pinMode(PIN_RELAY_AUX, OUTPUT);

  digitalWrite(PIN_RELAY_NC, HIGH);
  currentMillis = millis();

  ledcSetup(0, 1E5, 12);
  ledcAttachPin(PIN_BUZZER, 0);

  ledcWriteNote(0, NOTE_C, 5);
  delay(125);
  ledcWriteNote(0, NOTE_E, 5);
  delay(125);
  ledcWriteNote(0, NOTE_G, 5);
  delay(125);
  ledcWrite(0, 0);
  //OTA
  ArduinoOTA
    .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
    .onEnd([]() {
    Serial.println("\nEnd");
  })
    .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
    .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });


  //AutoConnect setup

  loadAux(AUX_ESPURNASETTING);
  loadAux(AUX_ESPURNASAVE);

  AutoConnectAux* setting = portal.aux(AUX_ESPURNASETTING);
  if (setting) {
    PageArgument  args;
    AutoConnectAux& espurna_setting = *setting;
    loadParams(espurna_setting, args);
    AutoConnectInput&     e_hostname = espurna_setting["espurnaserver"].as<AutoConnectInput>();
    AutoConnectInput&     e_apikey = espurna_setting["espurnaapikey"].as<AutoConnectInput>();
    config.bootUri = AC_ONBOOTURI_HOME;
    config.homeUri = "/";
    portal.config(config);

    espurnaServer = e_hostname.value;
    espurnaApiKey = e_apikey.value;

    Serial.println("server set to " + espurnaServer);
    Serial.println("apikey set to " + espurnaApiKey);

    portal.on(AUX_ESPURNASETTING, loadParams);
    portal.on(AUX_ESPURNASAVE, saveParams);
  }
  else
    Serial.println("aux. load error");



  Serial.print("WiFi ");
  if (portal.begin()) {
    Serial.println("connected:" + WiFi.SSID());
    Serial.println("IP:" + WiFi.localIP().toString());
  } else {
    Serial.println("connection failed:" + String(WiFi.status()));
    while (1) {
      delay(100);
      yield();
    }
  }

  WiFiWebServer&  webServer = portal.host();
  webServer.on("/", handleRoot);
  ArduinoOTA.begin();  

  hookState = OFFHOOK;
  onHook();
}

void loop() 
{

  currentMillis = millis();
  bool currentHookStatus = readHookState();

  if (currentHookStatus == OFFHOOK)
    offHook();
  else if (currentHookStatus == ONHOOK)
    onHook();


  if (hookState == ONHOOK && onHookMillis < currentMillis && onHookMillis > 0)
  {
    stopDevice();
    onHookMillis = 0;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    ArduinoOTA.handle();
  }
  portal.handleClient();
}