// SRPT-02 Repeater © 2023 by Red's Engineering is licensed under 
//CC BY-NC 4.0. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/4.0/
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <HTTPClient.h>
#include "helper.h"
#include <esp_sleep.h>
#include "OneButton.h"
#include <driver/adc.h>
#include "morse.h"
#include "PhoneDTMF.h"
#include "driver/rtc_io.h"
#include "uptime.h"
RTC_DATA_ATTR int sleepCount = 0;
RTC_DATA_ATTR int powerStatus = 1;
#define FORMAT_ON_FAIL true
using WebServerClass = WebServer;
#ifdef AUTOCONNECT_USE_LITTLEFS
#include <LittleFS.h>
#if defined(ARDUINO_ARCH_ESP8266)
FS &FlashFS = LittleFS;
#elif defined(ARDUINO_ARCH_ESP32)
fs::LittleFSFS &FlashFS = LittleFS;
#endif
#else
#include <FS.h>
#include <SPIFFS.h>
fs::SPIFFSFS &FlashFS = SPIFFS;
#endif
#define PARAM_FILE "/repeater.json"
#define USERNAME "abc" // For HTTP authentication
#define PASSWORD "123" // For HTTP authentication
#define HOST_NAME "SRPT-02"
String newHostname = "SRPT-02";
String stationID = "TEST";

int stationIDDelay;
int powerSavingMode = 0;
int enableStationID = 0;
int enableDebug = 0;
int wpm = 20;
int dtmfPriority = 1;
int configmode = 0;
int enableRepeater = 1;
int carrierDetected = 0;
unsigned long WaitingTimer; // timer variable for non-blocking timing
unsigned long oneSecondTimer;
unsigned long oneSecondTimer1;
const char *fw_ver = "Current Version: V1.2.0";
volatile byte state = LOW;
unsigned long StartTime = 0;
unsigned long CurrentTime = 0;
unsigned long ElapsedTime = 0;
unsigned long StartTime1 = 0;
unsigned long CurrentTime1 = 0;
unsigned long ElapsedTime1 = 0;
unsigned long StartTime3 = millis();
unsigned long CurrentTime3 = 0;
unsigned long ElapsedTime3 = 0;
int calibrate = 0;
int sleepEnable = 1;
int enabledStationID = 0;
int dtmfDetected = 0;
long timer = 0;
int DTMFConfigMode = 0;
int carrierDetectDelay = 100;
int minimumMessageLength = 1000;
#define BUTTON_PIN_BITMASK 0x000000000 // 2^33 in hex

// New dtmf
PhoneDTMF dtmf = PhoneDTMF();
uint8_t analogPin = 10;
String test;
unsigned long current_dac_millis, start_dac_millis;
Morse morse_dac(M_DAC, DAC_CHANNEL_1); // use DAC channel 1
// backticks will toggle digraph sending
// String		 tx = "ab7cd de a7bcd k `ar`";
// Setup a new OneButton on pin 8 carrier detect
OneButton power = OneButton(
    8,    // Input pin for the button
    true, // Button is active LOW
    true  // Enable internal pull-up resistor
);

OneButton carrierDetect = OneButton(
    14,    // Input pin for radio carrier detect
    false, // Button is active LOW
    false  // Enable internal pull-up resistor
);

OneButton button = OneButton(
    0,    // Input pin for the Setup button
    true, // Button is active LOW
    true  // Enable internal pull-up resistor
);

WebServerClass server;
AutoConnect portal(server);
AutoConnectConfig config;
AutoConnectAux repeaterAux;
AutoConnectAux saveAux;

void powerSwitch(void)
{

  if (powerStatus == 1)
  {
    digitalWrite(11, LOW);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    Serial.println("Entering Sleep...");
    digitalWrite(9, LOW);
    digitalWrite(34, LOW);
    digitalWrite(3, LOW);
    digitalWrite(11, LOW);
    sleepCount = 1;
    powerStatus = 0;
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 1);

    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 1);
    //  esp_sleep_enable_ext1_wakeup(0x101,ESP_EXT1_WAKEUP_ALL_LOW);
    // esp_sleep_disable_wakeup_source(esp_sleep_ext1_wakeup_mode_t);
    // esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_8, 0);
    esp_wifi_stop();
    // rtc_gpio_hold_dis(GPIO_NUM_10);
    rtc_gpio_hold_dis(GPIO_NUM_10);
    rtc_gpio_hold_dis(GPIO_NUM_0);
    rtc_gpio_isolate(GPIO_NUM_10);
    rtc_gpio_isolate(GPIO_NUM_0);
    delay(100);
    esp_deep_sleep_start();
  }

  else
  {
    digitalWrite(11, HIGH);
    rtc_gpio_hold_dis(GPIO_NUM_10);
    rtc_gpio_hold_dis(GPIO_NUM_0);
    powerStatus = 1;
  }
}

void deleteAllCredentials(void)
{
  // Delete all WiFi settings in nvram.
  AutoConnectCredential credential;
  station_config_t config;
  uint8_t ent = credential.entries();

  Serial.println("AutoConnectCredential deleting");
  if (ent)
    Serial.printf("Available %d entries.\n", ent);
  else
  {
    Serial.println("No credentials saved.");
    return;
  }

  while (ent--)
  {
    credential.load((int8_t)0, &config);
    if (credential.del((const char *)&config.ssid[0]))
      Serial.printf("%s deleted.\n", (const char *)config.ssid);
    else
      Serial.printf("%s failed to delete.\n", (const char *)config.ssid);
  }
}

bool whileCP(void)
{
  bool rc = true;
  // Here, something to process while the captive portal is open.
  // To escape from the captive portal loop, this exit function returns false.
  // rc = true;, or rc = false;
  // Detect if button is pushed to exit from captive portal, reset the CPU and reload nex settings from xml
  CurrentTime3 = millis();
  ElapsedTime3 = CurrentTime3 - StartTime3;

 

  int exit = digitalRead(0);
  if (exit == 0)
  {

    delay(500);
    Serial.println("RESTARTING...");

    ESP.restart();
  }

  int powerOff = digitalRead(8);
  if (powerOff == 0)
  {

    delay(500);

    powerSwitch();
  }

  return rc;
}

// Assign global variables to values stored in xml
void setParams(AutoConnectAux &aux)
{

  stationID = aux[F("stationID")].as<AutoConnectInput>().value;
  stationIDDelay = aux[F("stationIDDelay")].as<AutoConnectInput>().value.toInt();
  powerSavingMode = aux[F("powerSavingMode")].as<AutoConnectCheckbox>().checked;
  enableDebug = aux[F("enableDebug")].as<AutoConnectCheckbox>().checked;
  enableStationID = aux[F("enableStationID")].as<AutoConnectCheckbox>().checked;
  carrierDetectDelay = aux[F("carrierDetectDelay")].value.toInt();
  minimumMessageLength = aux[F("minimumMessageLength")].value.toInt();
  wpm = aux[F("wpm")].as<AutoConnectSelect>().value().toInt();
}
// Used to load xml file containing repeater settings from flash

bool loadParams(AutoConnectAux &aux)
{
  bool rc = false;

  Serial.print(PARAM_FILE);
  File param = FlashFS.open(PARAM_FILE, "r");
  if (param)
  {
    // These parameters are stored as JSON definitions in AutoConnectElements,
    // so the AutoConnectAux::loadElement function can be applied to restore.
    if (aux.loadElement(param))
    {

      // Reflects the loaded repeater settings to global variables.
      // call function to assign global variables to values stored in xml
      setParams(aux);
      Serial.println(" loaded");
      rc = true;
    }
    else
      Serial.println(" failed to load");
    param.close();
  }
  else
    Serial.println(" open failed");
  return rc;
}

// Config button pressed
void buttonstart()
{
  Serial.println("Setup Button Pressed...");

  DTMFConfigMode = 0;
  sleepEnable = 0;
  setCpuFrequencyMhz(240);
  if (configmode == 1)
  {
    Serial.println("RESTARTING...");
    ESP.restart();
  }
  // ESP.restart();
  configmode = 1;
  Serial.println("Config Mode = ");
  Serial.println(configmode);
  Serial.println("Enterring Configurration Mode");

  digitalWrite(9, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(34, HIGH);

  digitalWrite(11, HIGH);
  if (enableDebug == 0)
    portal.disableMenu(AC_MENUITEM_OPENSSIDS | AC_MENUITEM_DISCONNECT);
  portal.begin();
}

// This function will be called when the radio detects a carrier present
void carrrierDetect()
{
  carrierDetected = 1;

  StartTime = millis();
  if (enableDebug == 1)
    Serial.println("Carrier Dectect longPress...");

  if (powerSavingMode == 0)
    digitalWrite(3, HIGH); // recorrd LED on
  digitalWrite(13, HIGH);  // record on

  if (enableDebug == 1)
  {
    int Freq = getCpuFrequencyMhz();
    Serial.print("CPU Freq = ");
    Serial.print(Freq);
    Serial.println(" MHz");
    Freq = getXtalFrequencyMhz();
    Serial.print("XTAL Freq = ");
    Serial.print(Freq);
    Serial.println(" MHz");
    Freq = getApbFrequency();
    Serial.print("APB Freq = ");
    Serial.print(Freq);
    Serial.println(" Hz");
  }
} // Press

// This function will be called once, once carrier detect has been lost
void carrrierDetectLost()
{
  carrierDetected = 0;
  CurrentTime = millis();
  ElapsedTime = CurrentTime - StartTime;

  digitalWrite(13, LOW); // record off

  digitalWrite(3, LOW); // record LED off
  delay(1000);
  if (enableDebug == 1)
    Serial.println("Carrier Detect Lost...");
  dac_output_enable(DAC_CHANNEL_1);

  if ((enableRepeater == 1) || (dtmfDetected == 1))
  {

    digitalWrite(4, HIGH); // transmit on
    if (dtmfDetected == 0)
      digitalWrite(12, HIGH); // play on

    if (powerSavingMode == 0)
    {
      digitalWrite(9, HIGH);
      digitalWrite(34, HIGH);
    }

    if (ElapsedTime > 10000)
      ElapsedTime = 10000;
    if (dtmfDetected == 0)
      delay(ElapsedTime);

    digitalWrite(12, LOW); // play off
    digitalWrite(34, LOW);
    timer = millis();
    StartTime = millis();

    morse_dac.dac_set_wpm(wpm);

    if ((enableStationID == 1) && (dtmfDetected == 0))
    {
      dtmfPriority = 0;
      delay(stationIDDelay);
      morse_dac.dac_tx(stationID);
    }
  }

  if (dtmfDetected == 1)
  {
    delay(1000);
    if (enableRepeater == 1)
      morse_dac.dac_tx("S");
    else
      morse_dac.dac_tx("O");
  }
  dtmfDetected = 0;
  dtmfPriority = 0;

} // PressStop

void resettimer()
{
  StartTime1 = millis();
  ;
  ElapsedTime1 = 0;
  Serial.println(ElapsedTime1);
}

void factoryreset()
{
  CurrentTime1 = millis();
  ElapsedTime1 = CurrentTime1 - StartTime1;

  if (ElapsedTime1 > 5000)
  {
    Serial.println("Factory Reset");
    digitalWrite(3, HIGH);
    digitalWrite(34, HIGH);
    digitalWrite(9, HIGH);
    digitalWrite(11, HIGH);
    delay(1000);
    digitalWrite(3, LOW);
    digitalWrite(34, LOW);
    digitalWrite(9, LOW);
    digitalWrite(11, LOW);
    delay(1000);
    digitalWrite(3, HIGH);
    digitalWrite(34, HIGH);
    digitalWrite(9, HIGH);
    digitalWrite(11, HIGH);
    delay(1000);
    bool formatted = SPIFFS.format();
    delay(1000);
    deleteAllCredentials();
    delay(1000);
    ESP.restart();
  }
}

void detectDTMF()
{
  uint8_t tones;
  char button;
  // detect tone
  tones = dtmf.detect();
  // if valid tone was found, proof for validity
  button = dtmf.tone2char(tones);
  if (button > 0)
  {
    if (enableDebug == 1)
      Serial.print("Detected tone...");
    // measure 4 times, result of each measurement should be always the same
    // time need for this process: 80ms, so the tone must be present at least 100ms to be valid
    // tones |= dtmf.detect() | dtmf.detect();
    // tones = dtmf.detect();
    if (dtmf.tone2char(tones) > 0)
    {
      button = dtmf.tone2char(tones);
      if (enableDebug == 1)
      {
        Serial.print(" Detected '");
        Serial.print(button);
      }
      if ((button == '*') && (digitalRead(14) == 1))
      {

        if (enableDebug == 1)
          Serial.print("Repeater is Enabled...");
        digitalWrite(3, LOW);
        digitalWrite(34, LOW);
        digitalWrite(9, LOW);
        enableRepeater = 1;
        dtmfDetected = 1;
      }

      if (button == '#')
      {
        if (enableDebug == 1)
          Serial.print("Repeater is Disabled...");
        digitalWrite(3, LOW);
        digitalWrite(34, LOW);
        digitalWrite(9, LOW);
        enableRepeater = 0;
        dtmfDetected = 1;
      }

      if (button == '1')
      {
        digitalWrite(3, LOW);
        digitalWrite(34, LOW);
        digitalWrite(9, LOW);
        dtmfDetected = 1;
      }

      if (button == '2')
      {
        DTMFConfigMode = 1;
        digitalWrite(3, LOW);
        digitalWrite(34, LOW);
        digitalWrite(9, LOW);
        dtmfDetected = 1;
      }
    }
    else
    {
      if (enableDebug == 1)
      {
        Serial.print(" Dismiss '");
        Serial.print(button);
        Serial.println("' (invalid)");
      }
    }
  }
}

void setup()
{
  powerStatus = 1;

  rtc_gpio_hold_dis(GPIO_NUM_0);
  rtc_gpio_hold_dis(GPIO_NUM_10);

  if (enableDebug == 1)
    Serial.begin(115200);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();

  config.title = "Red's Engineering  SRPT-02";
  config.bootUri = AC_ONBOOTURI_ROOT;
  config.ota = AC_OTA_BUILTIN;
  config.apid = "SRPT-02";
  config.psk = "";
  config.auth = AC_AUTH_NONE;
  config.authScope = AC_AUTHSCOPE_AUX;
  config.username = USERNAME;
  config.password = PASSWORD;
  config.ticker = true;
  config.autoReconnect = true;
  config.reconnectInterval = 1;
  config.otaExtraCaption = fw_ver;
  portal.home("/repeater");
  portal.whileCaptivePortal(whileCP);
  portal.join({repeaterAux, saveAux});
  portal.config(config);
  // setting DTMF properties
  adc1_config_width(ADC_WIDTH_BIT_13);
  // dtmf.begin((uint8_t)ADC1_CHANNEL_9); // Use ADC 1, Channel 2 (GPIO3 on ESP32-SOLO)
  dtmf.begin(10, 0);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(34, OUTPUT);
  pinMode(0, INPUT_PULLUP);
  pinMode(14, INPUT_PULLDOWN);
  pinMode(4, OUTPUT);
  pinMode(8, INPUT_PULLUP);
  digitalWrite(4, LOW);
  // Status LED's
  pinMode(3, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(9, OUTPUT);
  int cal = digitalRead(8);
  if (cal == 0)
    calibrate = 1;

  // Responder of root page handled directly from WebServer class.

  server.on("/", []()
            { server.send(200, "text/html", String(F("<html>"
                                                     "<head><meta http-equiv=\"refresh\" content=\"1; url='/repeater'\" /></head>"
                                                     "</body>"
                                                     "</html>"))); });

  // Load a custom web page described in JSON as PAGE_ELEMENT and
  // register a handler. This handler will be invoked from
  // AutoConnectSubmit named the Load defined on the same page.
  FlashFS.begin(FORMAT_ON_FAIL);
  repeaterAux.load(FPSTR(REPEATER_ELEMENTS));

  File param = FlashFS.open(PARAM_FILE, "r");
  if (param)
  {
    repeaterAux.loadElement(param, {"product", "stationID", "wpm", "powerSavingMode", "stationIDDelay", "enableDebug", "enableStationID", "carrierDetectDelay", "minimumMessageLength"});

    param.close();
  }

  saveAux.load(FPSTR(PAGE_SAVE));
  saveAux.on([](AutoConnectAux &aux, PageArgument &arg)
             {
               aux["caption"].value = PARAM_FILE;

               File param = FlashFS.open(PARAM_FILE, "w");
               if (param)
               {
                 // Save as a loadable set for parameters.
                 repeaterAux.saveElement(param, {"product", "stationID", "wpm", "powerSavingMode", "stationIDDelay", "enableDebug", "enableStationID", "carrierDetectDelay", "minimumMessageLength"});
                 param = FlashFS.open(PARAM_FILE, "r");
                if(enableDebug ==1)
                aux["echo"].value = param.readString();
                 param.close();

                 // reload xml file with new settings done in debug mode only
                 //  if(enableDebug == 1){
                 //   AutoConnectAux& settings = *portal.aux("/repeater");
                 //   loadParams(settings);
                 //  }
               }
               else
               {
                 aux["echo"].value = "Filesystem failed to open.";
               }
               return String(); });

  dac_cw_config_t dac_cw_config;
  dac_cw_config.scale = DAC_CW_SCALE_2;
  // dac_output_voltage(DAC_CHANNEL_1, 125);
  dac_cw_config.phase = DAC_CW_PHASE_180;
  dac_cw_config.freq = 550;

  // Load settings from xml
  AutoConnectAux &settings = *portal.aux("/repeater");
  loadParams(settings);
  button.attachLongPressStart(resettimer);
  button.attachLongPressStop(factoryreset);
  carrierDetect.attachLongPressStart(carrrierDetect);
  carrierDetect.attachLongPressStop(carrrierDetectLost);
  carrierDetect.setPressTicks(carrierDetectDelay); // Delay for carrier detect
  button.attachClick(buttonstart);
  power.attachClick(powerSwitch);
  // Set new WPM
  Morse(0, 3, wpm);
  if (enableDebug == 0)
  {
    portal.disableMenu(AC_MENUITEM_CONFIGNEW);
    config.immediateStart = true;
  }

  if (enableDebug == 0)
  {
  }

  if (powerSavingMode == 0)
    digitalWrite(11, HIGH);

  if (powerSavingMode == 1)
  {

    digitalWrite(11, HIGH);
    delay(700);
    digitalWrite(11, LOW);
    delay(700);
    digitalWrite(11, HIGH);
    delay(700);
    digitalWrite(11, LOW);
    delay(700);
    digitalWrite(11, HIGH);
    delay(700);
    digitalWrite(11, LOW);
    delay(700);
  }

  if (enableDebug == 1)
    sleepEnable = 0;

  if ((sleepEnable == 1) && (DTMFConfigMode == 0))
  {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 1);
    esp_sleep_enable_ext1_wakeup(257, ESP_EXT1_WAKEUP_ALL_LOW);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
  }
}

void loop()
{
  // disable carrier detect when in config mode
  if (configmode == 0)
    carrierDetect.tick();
  if (digitalRead(14) == 0)
  {
    button.tick();
    power.tick();
  }
  if (configmode == 1)
    portal.handleClient();
  if ((dtmfPriority == 0) && (configmode == 0))
    morse_dac.dac_watchdog();

  if (digitalRead(14) == 1)
    detectDTMF();

  if (TimePeriodIsOver1(oneSecondTimer1, 10000))

  {

    if (enableDebug == 1)
    {

      uptime::calculateUptime();
      Serial.print("hours: ");
      Serial.println(uptime::getHours());

      Serial.print("minutes: ");
      Serial.println(uptime::getMinutes());
      Serial.print("seconds: ");
      Serial.println(uptime::getSeconds());

      Serial.println("***************************************************************************************");
      Serial.println("dtmfDetected");
      Serial.println(dtmfDetected);
      Serial.println("dtmfPriority");
      Serial.println(dtmfPriority);

      uint32_t rate = dtmf.getSampleFrequence();
      Serial.println("samplerate");
      Serial.println(rate);
    }
  }

  if (TimePeriodIsOver(oneSecondTimer, 600))
  {

    if ((!morse_dac.dac_transmitting() && digitalRead(4) == 1))
    {

      digitalWrite(4, LOW); // transmit off
      if (enableDebug == 1)
        Serial.println("Transmit off!!!!!");

      // Turn off LED's
      digitalWrite(9, LOW);
      digitalWrite(3, LOW);
      digitalWrite(34, LOW);
      if (powerSavingMode == 0)
        digitalWrite(11, HIGH);
      dtmfPriority = 1;
      dtmfDetected = 0;
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      esp_wifi_stop();

      if ((sleepEnable == 1) && (DTMFConfigMode == 0))
      {

        // digitalWrite(11, LOW);
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 1);
        esp_sleep_enable_ext1_wakeup(257, ESP_EXT1_WAKEUP_ALL_LOW);
        esp_sleep_enable_gpio_wakeup();
        esp_light_sleep_start();
        sleepCount = 0;
        dtmfPriority = 1;
      }
    }
  }
}
