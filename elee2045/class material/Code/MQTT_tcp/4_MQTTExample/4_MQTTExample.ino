#include "M5Unified.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "Preferences.h"
#include "WifiFunctions.h"
Preferences preferences;
M5Canvas canvas(&M5.Display); 

WiFiClient wifiClient; 

PubSubClient mqttClient(wifiClient);

#pragma pack(1)
typedef struct { uint8_t pressed; uint32_t t; } ButtonMessage;
#pragma pack(1)
typedef struct { float r; uint32_t t; } RandomMessage;
ButtonMessage buttonMessage; RandomMessage randomMessage;

char * BUTTON_TOPIC = "elee2045sp26/button/kjohnsen_arduino";
char * RANDOM_TOPIC = "elee2045sp26/random/kjohnsen_python";
char * TEXT_TOPIC = "elee2045sp26/text_topic/kjohnsen_python";
char username[50];
char password[50];
char ssid[50];
char text_buffer[100];
void mqttMessageCallback(char * topic, uint8_t* payload, unsigned int length){
  if(strcmp(topic,RANDOM_TOPIC) == 0 && length == sizeof(RandomMessage)){
    memcpy(&randomMessage,payload,length);
    canvas.clear();
    canvas.setCursor(10,10); canvas.print("Got Random Message: "); canvas.println(randomMessage.r);
    canvas.setCursor(10,30); canvas.println(randomMessage.t);
    
  }else if(strcmp(topic,TEXT_TOPIC) == 0 && length < 100){
    //canvas.clear();
    //canvas.setCursor(10,50); canvas.print("Got Text Message of size: "); canvas.println(length);
    memcpy(text_buffer,payload,length);
    text_buffer[length] = 0; //last character of string
    canvas.setCursor(10,70); canvas.println(text_buffer);

  }
  else {
    canvas.setCursor(10,50); canvas.print("Got Unknown message "); canvas.println(length);
  }
  canvas.pushSprite(0,0);
}

void connectToMQTT() {
  canvas.clear();
  canvas.setCursor(10,10); canvas.println("connecting mqtt");
  if (!mqttClient.connect("kyle's m5stick","giiuser","giipassword")) { // must be unique username
    canvas.setCursor(10,30); canvas.print("mqtt failed, rc="); canvas.println(mqttClient.state());
  } else {
    canvas.setCursor(10,30); canvas.println("mqtt connected");
    mqttClient.subscribe(RANDOM_TOPIC);
    mqttClient.subscribe(TEXT_TOPIC);
  }
  canvas.pushSprite(0,0);
}

void setup() {
  M5.begin();
  M5.Display.setRotation(1);
  canvas.createSprite(M5.Display.width(),M5.Display.height());

  mqttClient.setServer("eduoracle.ugavel.com",1883);
  mqttClient.setCallback(mqttMessageCallback);

  preferences.begin("wifi", true);
  preferences.getString("ssid",ssid,50);
  preferences.getString("username",username,50);
  preferences.getString("password",password,50);
}

void loop() {
  M5.update();

  if(WiFi.status() != WL_CONNECTED) connectToWifi(ssid,username,password);
  if(!mqttClient.connected()) connectToMQTT();

  if(M5.BtnA.wasPressed()){
    buttonMessage.pressed = 1; buttonMessage.t=millis();
    mqttClient.publish(BUTTON_TOPIC,(uint8_t*)&buttonMessage,sizeof(ButtonMessage));
  }
  if(M5.BtnA.wasReleased()){
    buttonMessage.pressed = 0; buttonMessage.t=millis();
    mqttClient.publish(BUTTON_TOPIC,(uint8_t*)&buttonMessage,sizeof(ButtonMessage));
  }

  mqttClient.loop();
}