#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#define GET_CHIPID() ((uint16_t)(ESP.getEfuseMac() >> 32))
#include <FS.h>
#include <AutoConnect.h>
#include <ArduinoOTA.h>

#define HOST_NAME "fumebuddy" + GET_CHIPID()

// AutoConnect stuff
#define PARAM_FILE    "/param.json"
#define AUX_SETTINGS  "/fumebuddy_setting"
#define AUX_SAVED     "/fumebuddy_save"

// Fumebuddy Definitions
#define GPIO_SENSOR     17
#define GPIO_RELAY_AUX  14
#define GPIO_RELAY_NC   5
#define GPIO_RELAY_NO   12
#define GPIO_BUZZER     26
#define TOUCH_INPUT     T7
#define TOUCH_THRESHOLD 50
#define TOUCH_MINLENGTH 250
#define OFFHOOK HIGH
#define ONHOOK LOW

enum class HookState
{
  STATE_OFFHOOK,      // Off hook
  STATE_ONHOOK_READY, // Ready for On Hook
  STATE_ONHOOK        // On hook
};
bool hookState = ONHOOK;
uint32_t currentMillis = 0;
uint32_t onHookMillis = 0;
HookState currentState;
bool triggerTouch = false, handledTouch = false;

typedef WebServer WiFiWebServer;
uint32_t touchMillis;
AutoConnect portal;
AutoConnectConfig config;
WiFiClient wifiClient;

String fumebuddyOn, fumebuddyOff, fumebuddyToggle;
unsigned int fumebuddyDelay;
bool fumebuddyBeeper;


HTTPClient http;

bool readHookState()
{
  return digitalRead(GPIO_SENSOR);
}

void startDevice()
{
  http.begin(fumebuddyOn);
  http.GET();
  http.end();

  if (fumebuddyBeeper)
  {
    ledcWriteNote(0, NOTE_C, 5);
    delay(125);
    ledcWriteNote(0, NOTE_D, 5);
    delay(125);
    ledcWrite(0, 0);
  }
}

void stopDevice()
{
  http.begin(fumebuddyOff);
  http.GET();
  http.end();

  if (fumebuddyBeeper)
  {
    ledcWriteNote(0, NOTE_D, 5);
    delay(125);
    ledcWriteNote(0, NOTE_C, 5);
    delay(125);
    ledcWrite(0, 0);
  }
}

void toggleDevice()
{
  Serial.println("Toggle time!");
  http.begin(fumebuddyToggle);
  http.GET();
  http.end();
  if (fumebuddyBeeper)
  {
    ledcWriteNote(0, NOTE_C, 5);
    delay(125);
    ledcWriteNote(0, NOTE_C, 5);
    delay(125);
    ledcWrite(0, 0);
  }
}

void offHook()
{
  if (hookState == ONHOOK)
  {
    digitalWrite(GPIO_RELAY_NO, LOW);
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
    digitalWrite(GPIO_RELAY_NO, HIGH);
    hookState = ONHOOK;
    onHookMillis = millis() + (fumebuddyDelay * 1000L);
    Serial.println("Going on hook");
  }
}

String loadParams(AutoConnectAux &aux, PageArgument &args)
{
  (void)(args);
  File param = SPIFFS.open(PARAM_FILE, "r");
  if (param)
  {
    aux.loadElement(param);
    param.close();
  }
  else
    Serial.println(PARAM_FILE " open failed");
  return String("");
}

String saveParams(AutoConnectAux &aux, PageArgument &args)
{

  fumebuddyOn = args.arg("urlon");
  fumebuddyOn.trim();

  fumebuddyOff = args.arg("urloff");
  fumebuddyOff.trim();

  fumebuddyToggle = args.arg("urltoggle");
  fumebuddyToggle.trim();

  String t_fumebuddyDelay = args.arg("delay");
  t_fumebuddyDelay.trim();
  fumebuddyDelay = t_fumebuddyDelay.toInt();

  String t_fumebuddyBeeper = args.arg("beeper");
  t_fumebuddyBeeper.trim();

  File param = SPIFFS.open(PARAM_FILE, "w");
  portal.aux("/fumebuddy_setting")->saveElement(param, {"urlon", "urloff", "urltoggle", "delay", "beeper"});
  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText &echo = aux["parameters"].as<AutoConnectText>();
  echo.value += "url on: " + fumebuddyOn + "<br>";
  echo.value += "url off: " + fumebuddyOff + "<br>";
  echo.value += "url toggle: " + fumebuddyToggle + "<br>";
  echo.value += "delay : " + String(fumebuddyDelay) + "<br>";
  echo.value += "beeper: " + (fumebuddyBeeper == true ? String("true") : String("false")) + "<br>";

  return String("");
}

void handleRoot()
{
  String content =
      "<html>"
      "<head>"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "</head>"
      "<body>"
      "<p style=\"padding-top:10px;text-align:left\"> Settings: " AUTOCONNECT_LINK(COG_24) "</p>"
                                                                                           "</body>"
                                                                                           "</html>";

  WiFiWebServer &webServer = portal.host();
  webServer.send(200, "text/html", content);
}

bool loadAux(const String auxName)
{
  bool rc = false;
  String fn = auxName + ".json";
  File fs = SPIFFS.open(fn.c_str(), "r");
  if (fs)
  {
    rc = portal.load(fs);
    fs.close();
  }
  else
    Serial.println("SPIFFS open failed: " + fn);
  return rc;
}

void readTouch()
{
  int touchInput = touchRead(TOUCH_INPUT);
  // Touched and we weren't touched before
  if (touchInput < TOUCH_THRESHOLD && touchMillis == 0)
  {
    touchMillis = currentMillis;
  }
  // Still touched, was previously touched, longer then min touch length and hasn't been fired yet
  else if (touchInput < TOUCH_THRESHOLD && currentMillis - touchMillis > TOUCH_MINLENGTH && handledTouch == false)
    triggerTouch = true;

  // Not touched, reset everything
  if (touchInput > TOUCH_THRESHOLD)
  {
    touchMillis = 0;
    triggerTouch = false;
    handledTouch = false;
  }
}

void setup()
{
  delay(250);
  Serial.begin(115200);
  Serial.println();
  SPIFFS.begin();

  // Fumebuddy setup
  pinMode(GPIO_RELAY_NO, OUTPUT);
  pinMode(GPIO_RELAY_NC, OUTPUT);
  pinMode(GPIO_SENSOR, INPUT_PULLUP);
  pinMode(GPIO_RELAY_AUX, OUTPUT);
    
  // This is a bodge for GPIO25 on rev 3 versions of the board, so we don't affect the actual touch input of GPIO27.
  pinMode(25, INPUT);

  digitalWrite(GPIO_RELAY_NC, HIGH);
  currentMillis = millis();

  ledcSetup(0, 1E5, 12);
  ledcAttachPin(GPIO_BUZZER, 0);

  loadAux(AUX_SETTINGS);
  loadAux(AUX_SAVED);
  AutoConnectAux* setting = portal.aux(AUX_SETTINGS);

  // Setup AutoConfig
  config.bootUri = AC_ONBOOTURI_HOME;
  config.homeUri = "/";
  config.hostName = String("fumebuddy") + GET_CHIPID();
  portal.config(config);

  PageArgument args;
  AutoConnectAux& fumebuddy_setting = *setting;
  if (setting)
  {
  loadParams(fumebuddy_setting, args);
  AutoConnectInput &e_on = fumebuddy_setting["urlon"].as<AutoConnectInput>();
  AutoConnectInput &e_off = fumebuddy_setting["urloff"].as<AutoConnectInput>();
  AutoConnectInput &e_toggle = fumebuddy_setting["urltoggle"].as<AutoConnectInput>();
  AutoConnectInput &e_delay = fumebuddy_setting["delay"].as<AutoConnectInput>();
  AutoConnectCheckbox &e_beeper = fumebuddy_setting["beeper"].as<AutoConnectCheckbox>();

  fumebuddyOn = e_on.value;
  fumebuddyOff = e_off.value;
  fumebuddyToggle = e_toggle.value;
  fumebuddyDelay = e_delay.value.toInt();
  
  if (e_beeper.checked)
    fumebuddyBeeper = true;
  else
    fumebuddyBeeper = false;

  Serial.println("fumebuddyOn set to " + fumebuddyOn);
  Serial.println("fumebuddyOff set to " + fumebuddyOff);
  Serial.println("fumebuddyToggle set to " + fumebuddyToggle);
  Serial.print("fumebuddyDelay set to ");
  Serial.println(fumebuddyDelay);
  Serial.print("fumebuddyBeeper set to ");
  Serial.println(fumebuddyBeeper);
  Serial.println("hostname set to " + config.hostName);

  portal.on(AUX_SETTINGS, loadParams);
  portal.on(AUX_SAVED, saveParams);
  }
  else
    Serial.println("aux. load error");

  if (fumebuddyBeeper)
  {
    ledcWriteNote(0, NOTE_C, 5);
    delay(125);
    ledcWriteNote(0, NOTE_E, 5);
    delay(125);
    ledcWriteNote(0, NOTE_G, 5);
    delay(125);
    ledcWrite(0, 0);
  }


  Serial.print("WiFi\n");
  if (portal.begin())
  {
    Serial.println("connected:" + WiFi.SSID());
    Serial.println("IP:" + WiFi.localIP().toString());
  }
  else
  {
    Serial.println("connection failed:" + String(WiFi.status()));
    while (1)
    {
      delay(100);
      yield();
    }
  }

  //Setup OTA
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
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

    ArduinoOTA.begin();

  WiFiWebServer &webServer = portal.host();
  webServer.on("/", handleRoot);
  
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

  readTouch();

  if (triggerTouch)
  {
    toggleDevice();
    triggerTouch = false;
    handledTouch = true;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    ArduinoOTA.handle();
  }
  portal.handleClient();
}