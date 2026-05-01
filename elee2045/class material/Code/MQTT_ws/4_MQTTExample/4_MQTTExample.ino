#include "M5Unified.h"
#include "WiFi.h"
#include <mqtt_client.h>       // ESP-IDF built-in MQTT client (supports WebSockets)
#include <esp_crt_bundle.h>    // TLS certificate bundle for WSS
#include "Preferences.h"
#include "WifiFunctions.h"
Preferences preferences;
M5Canvas canvas(&M5.Display);

esp_mqtt_client_handle_t mqttClient = NULL;
bool mqttConnected = false;
bool mqttStarted = false;

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

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
      mqttConnected = true;
      canvas.clear();
      canvas.setCursor(10,10); canvas.println("mqtt connected");
      canvas.pushSprite(0,0);
      esp_mqtt_client_subscribe(mqttClient, RANDOM_TOPIC, 0);
      esp_mqtt_client_subscribe(mqttClient, TEXT_TOPIC, 0);
      break;
    case MQTT_EVENT_DISCONNECTED:
      mqttConnected = false;
      canvas.clear();
      canvas.setCursor(10,10); canvas.println("mqtt disconnected");
      canvas.pushSprite(0,0);
      break;
    case MQTT_EVENT_DATA:
      if(event->topic_len > 0){
        if(strncmp(event->topic, RANDOM_TOPIC, event->topic_len) == 0 && event->data_len == sizeof(RandomMessage)){
          memcpy(&randomMessage, event->data, event->data_len);
          canvas.clear();
          canvas.setCursor(10,10); canvas.print("Got Random Message: "); canvas.println(randomMessage.r);
          canvas.setCursor(10,30); canvas.println(randomMessage.t);
        }else if(strncmp(event->topic, TEXT_TOPIC, event->topic_len) == 0 && event->data_len < 100){
          memcpy(text_buffer, event->data, event->data_len);
          text_buffer[event->data_len] = 0;
          canvas.setCursor(10,70); canvas.println(text_buffer);
        }else{
          canvas.setCursor(10,50); canvas.print("Got Unknown message "); canvas.println(event->data_len);
        }
        canvas.pushSprite(0,0);
      }
      break;
    case MQTT_EVENT_ERROR:
      canvas.clear();
      canvas.setCursor(10,10); canvas.println("mqtt error");
      canvas.pushSprite(0,0);
      break;
    default:
      break;
  }
}

void connectToMQTT() {
  if(mqttStarted) return; // ESP-IDF client handles reconnection internally

  canvas.clear();
  canvas.setCursor(10,10); canvas.println("connecting mqtt via wss");
  canvas.pushSprite(0,0);

  // ESP-IDF v4.4 flat config format (Arduino ESP32 Core v2.x)
  esp_mqtt_client_config_t mqtt_cfg = {};
  mqtt_cfg.uri = "wss://eduoracle.ugavel.com/mqtt/";
  mqtt_cfg.username = "giiuser";
  mqtt_cfg.password = "giipassword";
  mqtt_cfg.crt_bundle_attach = esp_crt_bundle_attach;

  mqttClient = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqttClient, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  esp_mqtt_client_start(mqttClient);
  mqttStarted = true;
}

void setup() {
  M5.begin();
  M5.Display.setRotation(1);
  canvas.createSprite(M5.Display.width(),M5.Display.height());

  preferences.begin("wifi", true);
  preferences.getString("ssid",ssid,50);
  preferences.getString("username",username,50);
  preferences.getString("password",password,50);
}

void loop() {
  M5.update();

  if(WiFi.status() != WL_CONNECTED) connectToWifi(ssid,username,password);
  if(WiFi.status() == WL_CONNECTED && !mqttStarted) connectToMQTT();

  if(mqttConnected && M5.BtnA.wasPressed()){
    buttonMessage.pressed = 1; buttonMessage.t=millis();
    esp_mqtt_client_publish(mqttClient, BUTTON_TOPIC, (const char*)&buttonMessage, sizeof(ButtonMessage), 0, 0);
  }
  if(mqttConnected && M5.BtnA.wasReleased()){
    buttonMessage.pressed = 0; buttonMessage.t=millis();
    esp_mqtt_client_publish(mqttClient, BUTTON_TOPIC, (const char*)&buttonMessage, sizeof(ButtonMessage), 0, 0);
  }
}
