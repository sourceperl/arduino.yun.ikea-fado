/*
  MQTT led display
  Arduino Yun + Adafruit 12x NeoPixel ring (https://www.adafruit.com/products/1643) 

  publish to "test/color" a RGB hex encoded string (green is "00FF00") to update color of the NeoPixel
  publish to "test/millis" for receive current millis value on topic "test/duino/millis"
  
  color transition between 2 RGB values is fading
*/

#include <SPI.h>
#include <YunClient.h>
// MQTT lib available at https://github.com/knolleary/pubsubclient
#include <PubSubClient.h>
// NeoPixel lib available at https://github.com/adafruit/Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h>
// Timer lib available at https://github.com/JChristensen/Timer
#include "Timer.h"

// some consts
byte mqtt_broker[] = {192, 168, 1, 60};
#define MQTT_PORT             1883
#define MQTT_CLIENT_ID        "ArduinoClient"
#define MQTT_UPDATE_INTERVAL  200
#define PIXEL_PIN             6
#define PIXEL_COUNT           12
#define PIXEL_UPDATE_INTERVAL 50
#define LED_PIN               13

// some vars
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
YunClient yun;
PubSubClient mqtt(mqtt_broker, MQTT_PORT, mqtt_callback, yun);
Timer t;
byte red, _red     = 0;
byte green, _green = 0;
byte blue, _blue   = 0;

// transfer PSTR ("flash string") to RAM for use with char* params
/*
char txt_buf[64];
char* toRAM(const char* str) {
  snprintf_P(txt_buf, sizeof(txt_buf), str);
  return txt_buf;
}
*/

// call by MQTT lib for incoming messages
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // format pld (= payload) string
  char* pld = (char*)malloc(length+1);  
  memcpy(pld,payload,length);
  pld[length] = 0;
  // topic "test/duino" handler
  if (!strcmp_P(topic, PSTR("test/millis"))) {
    char buf[40];
    sprintf_P(buf, PSTR("millis=%lu"), millis()); 
    mqtt.publish("test/duino/millis", buf);  
  // topics handlers
  } else if (!strcmp_P(topic, PSTR("test/color"))) {
    // payload is RGB value in hexa (ex: red is "FF0000")    
    uint32_t color = strtol(pld, NULL, 16);
    red   = color>>16 & 0xff;
    green = color>>8  & 0xff;
    blue  = color     & 0xff;
  }
  // Free the memory
  free(pld);
}

void setup()
{
  // init Bridge
  Bridge.begin();
  // IO settings
  //pinMode(LED_PIN, OUTPUT);
  // init NeoPixel
  strip.begin();
  strip.show();
  // init timer job (after instead of every for regulary call delay)
  t.every(PIXEL_UPDATE_INTERVAL, jobPIX);
  t.every(MQTT_UPDATE_INTERVAL,  jobMQTT);
  //t.every(1000,                 jobLOG);
  jobPIX();
  jobMQTT();
}

void loop()
{
  // do timer job
  t.update();
}

// job NeoPixel
void jobPIX(void)
{
  // fade RGB progression
  one_step_to_target(&_red, red);
  one_step_to_target(&_green, green);
  one_step_to_target(&_blue, blue);
  // set RED color
  for(uint16_t i=0; i<strip.numPixels(); i++)
    strip.setPixelColor(i, strip.Color(_red, _green, _blue));
  strip.show();
}

// job MQTT
void jobMQTT(void)
{
  // do mqtt job, handle reconnect if need
  if(!mqtt.loop()) {
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      // publish/subscribe topics
      mqtt.publish("test/duino/msg","startup");
      mqtt.subscribe("test/millis");
      mqtt.subscribe("test/color");
    }
  }
}

/*
void jobLOG(void) {
  char a[20];
  ltoa(strip.Color(_red, _green, _blue), a, 16);
  mqtt.publish("test/duino/msg", a);
}
*/

void one_step_to_target(byte* value, byte target) {
  byte _step = 1;
  // diff and absolute diff
  int diff    = target - *value;
  byte a_diff = abs(diff);
  // step booster 
  if (a_diff > 20)
    _step++;
  if (a_diff > 5)
    _step++;
  // step by step
  if (diff > 0)
    *value += _step;  
  else if (diff < 0)
    *value -= _step;  
}


