#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include <AutoConnect.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include "espurna.h"

#define PARAM_FILE        "/param.json"
#define AUX_SETTING_URI   "/espurna_setting"
#define AUX_SAVE_URI      "/espurna_save"
#define AUX_CLEAR_URI     "/espurna_clear"

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

typedef WebServer WiFiWebServer;

bool hookState = ONHOOK;
uint32_t currentMillis = 0;
uint32_t onHookMillis = 0;

HTTPClient http;
WebServer server;
AutoConnect Portal(server);
AutoConnectConfig acConfig;
AutoConnectAux espurna_setting;

String serverName;
String apiKey;
HookState currentState;



bool readHookState()
{
  return digitalRead(PIN_SENSOR);
}



void startDevice()
{
  http.begin("http://" + serverName + "/api/relay/0?apikey=" + apiKey + "&value=1");
  http.GET();
  http.end();
}

void stopDevice()
{
  http.begin("http://" + serverName + "/api/relay/0?apikey=" + apiKey + "&value=0");
  http.GET();
  http.end();
}

void toggleDevice()
{
  http.begin("http://" + serverName + "/api/relay/0?apikey=" + apiKey + "&value=2");
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


// Load parameters saved with  saveParams from SPIFFS into the
// elements defined in /mqtt_setting JSON.
String loadParams(AutoConnectAux& aux, PageArgument& args)
{
  (void)(args);
  File param = SPIFFS.open(PARAM_FILE, "r");
  if (param)
  {
    if (aux.loadElement(param))
      Serial.println(PARAM_FILE " loaded");
    else
      Serial.println(PARAM_FILE " failed to load");
    param.close();
  }
  else
  {
    Serial.println(PARAM_FILE " open failed");
    Serial.println("If you get error as 'SPIFFS: mount failed, -10025', Please modify with 'SPIFFS.begin(true)'.");
  }
  return String("");
}


String saveParams(AutoConnectAux& aux, PageArgument& args)
{
  AutoConnectInput& espurnaserver = espurna_setting.getElement<AutoConnectInput>("espurnaserver");
  serverName = espurnaserver.value;
  serverName.trim();

  AutoConnectInput& apikey = espurna_setting.getElement<AutoConnectInput>("apikey");
  apiKey = apikey.value;
  apiKey.trim();

  File param = SPIFFS.open(PARAM_FILE, "w");
  espurna_setting.saveElement(param, { "espurnaserver", "apikey" });
  param.close();
  
  AutoConnectText&  echo = aux.getElement<AutoConnectText>("parameters");
  echo.value = "Server: " + serverName;
  echo.value += "API Key: " + apiKey + "<br>";
  
  return String("");
}

void handleRoot()
{
  String  content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "</head>"
    "<body>"
    "<iframe width=\"450\" height=\"260\" style=\"transform:scale(0.79);-o-transform:scale(0.79);-webkit-transform:scale(0.79);-moz-transform:scale(0.79);-ms-transform:scale(0.79);transform-origin:0 0;-o-transform-origin:0 0;-webkit-transform-origin:0 0;-moz-transform-origin:0 0;-ms-transform-origin:0 0;border: 1px solid #cccccc;\" src=\"https://thingspeak.com/channels/454951/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&type=line\"></iframe>"
    "<p style=\"padding-top:5px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
    "</body>"
    "</html>";

  WiFiWebServer&  webServer = Portal.host();
  webServer.send(200, "text/html", content);
}


void setup()
{
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
    
  Serial.begin(115200);

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

  SPIFFS.begin(true);

  
  acConfig.apid = "ESP-fumebuddy";
  acConfig.homeUri = "/";
  acConfig.portalTimeout = 300; // Timeout after 5 minutes
  Portal.config(acConfig);
  
  espurna_setting.load(AUX_espurna_settings);
  Portal.join({espurna_setting});

  Portal.on(AUX_SETTING_URI, loadParams);
  Portal.on(AUX_SAVE_URI, saveParams);
  //Portal.on("/", handleRoot);
  
  Portal.begin();
  ArduinoOTA.begin();

  
  //AutoConnectInput&     ac_serverName = espurna_setting->getElement<AutoConnectInput>("espurnaserver");
  //AutoConnectInput&     ac_apiKey = espurna_setting->getElement<AutoConnectInput>("apikey");
  
  //if (ac_serverName.value.length())
//    serverName = ac_serverName.value;
  //apiKey = ac_apiKey.value;
  
  

  //offHook();
  //delay(1000);
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
  Portal.handleClient();
}