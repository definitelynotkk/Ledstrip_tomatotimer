#include <Arduino.h> //for platformio
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#include <FastLED.h>
#include <SimpleTimer.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// #include <RemoteDebug.h>

#define NUM_LEDS 91 //number of leds
#define LED_PIN D4   //Use GPIO2 D4

/**setup MQTT and wifi**/
const char *ssid = "ssid";
const char *password = "password";
const char *mqtt_server = "IP";
const int mqtt_port = 1883;
const char *mqtt_username = "hassio";
const char *mqtt_password = "mqtt";
const char *mqtt_client_name = "ledCountdown";
CRGB leds[NUM_LEDS];
const EOrder rgbType = GRB; //change RGB order if color isn't correct.
WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;


int focusTimer = 1;
int restTimer = 2;
bool isFirstboot = true;
bool powerOn = false;
int mode = 0;
int cycle = 0;
int blinkVal = 50;

int timerseconds = 0;
int ledcountNo = 0;


//**timer configeration**/
long focustime = 1200000;  
//tomato timer 
//in milliseconds
//minutes multiply by 60000 to get milliseconds
int countdownTime = 3300; 
// short break timer
// actual time = countdowntime*number of leds
// in milliseconds

bool accending = true;
int brightness = 0;

void setup_wifi()
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  int connect = 0;
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(++connect);Serial.print(" ");
  }

  Serial.println("\n");
  Serial.println("Connection established");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  if (!client.connected())
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      setup_wifi();
    }
    if (client.connect(mqtt_client_name, mqtt_username, mqtt_password))
    {
      if (isFirstboot == true)
      {
        client.publish("ledstrip/desk/timer/status", "Rebooted");
        isFirstboot = false;
      }
      if (isFirstboot == false)
      {
        client.publish("ledstrip/desk/timer/status", "Reconnected");
      }
      client.subscribe("ledstrip/desk/timer");
      client.subscribe("ledstrip/desk/focusmode");
      client.subscribe("ledstrip/desk/gamingmode");
      client.subscribe("ledstrip/desk/power");
      //subscribe to mqtt topic
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String newPayload = String((char *)payload);
  String mqttTopic = topic;
  Serial.println(mqttTopic);
  Serial.println(newPayload);
  if (mqttTopic == "ledstrip/desk/power" && newPayload == "off")
  {
    powerOn = false;
    mode = 0;
  }
  if (mqttTopic == "ledstrip/desk/focusmode")
  {
    powerOn = true;
    nscale8(leds, NUM_LEDS, 1);
    mode = 1;
    cycle = 0;
    brightness = 1;
    reset();
  }
  if (mqttTopic == "ledstrip/desk/gamingmode")
  {

    mode = 2;
    reset();

  }
}

void subtractInterval()
{
  ledcountNo--;
}

void checkIn()
{
  client.publish("/ledstrip/timer/status", "OK");
}

void chooseMode()
{
  switch (mode)
  {
  case 0:
    reset();
    break;
  case 1:
    focusMode();
    break;
  case 2:
    blinking(40, 197, 255);
    break;
  }
}

void focusMode()
{
  if (cycle == 0)
  {
    nscale8(leds, NUM_LEDS, 50);
    focusTimer = timer.setTimeout(focustime, timesUp);
    cycle += 1;
  }
  if (cycle == 1)
  {
    focusing(40, 35, 30);
    timer.run();
  }
  if (cycle < 4 && cycle > 1)
  {
    blinking(0, 35, 30);
    // Serial.print(cycle);
  }
  if (cycle == 4)
  {
    FastLED.delay(500);
    cycle += 1;
    Serial.print(cycle);
    Serial.print("cycle5 finish");
  }
  if (cycle < 8 && cycle > 4)
  {
    Serial.print("cycle6-8");
    blinking(0, 35, 30);
    Serial.print(cycle);
  }

  if (cycle == 8)
  {
    // Serial.print("cycle5 start");
    FastLED.delay(5000);
    restTimer = timer.setTimer(countdownTime, subtractInterval, NUM_LEDS);
    cycle += 1;
    // Serial.print("cycle5 finish");
  }

  if (cycle == 9)
  {
    ledCountdown();
    timer.run();
  }
}

void timesUp()
{
  timer.disable(focusTimer);
  timer.deleteTimer(focusTimer);
  cycle += 1;
  brightness = 1;
}

void ledCountdown()
{
  nscale8(leds, NUM_LEDS, 150);

  for (int i = 0; i < ledcountNo; i++)
  {
    int hue = (ledcountNo + 100);
    leds[i] = CHSV(hue, 150, 150);
  }
  //Serial.println(ledcountNo);
  FastLED.delay(50);

  if (ledcountNo == 0)
  {
    FastLED.delay(2000);
    reset();
    // Serial.print("full cycle finished");
    // Serial.print(cycle);
  }
}

  




void reset()
{
  ledcountNo = NUM_LEDS;
  timer.disable(restTimer);
  timer.disable(focusTimer);
  timer.deleteTimer(restTimer);
  timer.deleteTimer(focusTimer);
  nscale8(leds, NUM_LEDS, 1);
  cycle = 0;
}

void blinking(int speed, int hue, int sat)
{
  if (brightness > -1 && brightness < 160)
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CHSV(hue, sat, brightness);
    }
  }

  if (brightness < 1 && accending == false)
  {

    accending = true;
    cycle++;
    brightness = 1;
  }
  if (brightness > 160 && accending == true)
  {
    accending = false;
    brightness = 160;
  }
  if (accending == true)
  {
    brightness += 2;
  }
  if (accending == false)
  {
    brightness -= 2;
  }
  FastLED.delay(speed);
}

void focusing(int speed, int hue, int sat)
{
  int i = NUM_LEDS - 1;

  leds[i] = CHSV(hue, sat, brightness);

  if (brightness == 0 && accending == false)
  {
    accending = true;
    brightness = 1;
  }
  if (brightness == 180 && accending == true)
  {
    accending = false;
    brightness = 180;
  }
  if (accending == true)
  {
    brightness++;
  }
  if (accending == false)
  {
    brightness--;
  }
  FastLED.delay(speed);
  //Serial.println(cycle);  
}

void setup()
{
  Serial.begin(115200);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  FastLED.addLeds<WS2812B, LED_PIN, rgbType>(leds, NUM_LEDS);
  WiFi.mode(WIFI_STA);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  timer.setInterval(90000, checkIn);
  ArduinoOTA.setHostname("ledstrip");
  ArduinoOTA.begin();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
  chooseMode();
  FastLED.show();
  Serial.println(mode);
}