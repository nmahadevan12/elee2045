#include <M5StickCPlus2.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define REED_SWITCH 26

enum modeState { OFF = 0, HOME = 1, AWAY = 2 };
modeState currentState = OFF;

bool alarmTripped = false;
unsigned long tripStartTime = 0;
const unsigned long alarmDelay = 10000; 
int lastSecondsLeft = -1; 

// --- HEARTBEAT VARIABLES ---
unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 600000; // 10 minutes in ms
const char* HEARTBEAT_TOPIC = "elee2045sp26/nm25652/heartbeat";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const char* mqtt_server = "eduoracle.ugavel.com";
const char* SENSOR_TOPIC = "elee2045sp26/nm25652/doorSensor";
const char* MODE_TOPIC = "elee2045sp26/nm25652/mode";

#pragma pack(1)
typedef struct {
    uint8_t mode; 
    uint8_t open; 
    uint32_t t;
} ModeMessage;
ModeMessage modeMessage;

int lastState = -1;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (String(topic) == MODE_TOPIC && length >= 1) {
        currentState = (modeState)payload[0];
        alarmTripped = false;
        lastSecondsLeft = -1;
        StickCP2.Speaker.stop();

        StickCP2.Display.fillScreen(BLACK);
        StickCP2.Display.setCursor(0, 0);
        StickCP2.Display.setTextColor(RED);
        if (currentState == OFF) StickCP2.Display.println("Off");
        else if (currentState == HOME) StickCP2.Display.println("Home");
        else if (currentState == AWAY) StickCP2.Display.println("Away");
    }
}

void connectToMQTT() {
    StickCP2.Display.fillScreen(BLACK);
    if (mqttClient.connect("Nikhil_M5_Sensor", "giiuser", "giipassword")) {
        mqttClient.subscribe(MODE_TOPIC);
        delay(500);
        StickCP2.Display.fillScreen(BLACK);
    }
}

void modeSelect() {
    if (StickCP2.BtnA.wasPressed()) {
        currentState = (modeState)((currentState + 1) % 3);
        alarmTripped = false;
        lastSecondsLeft = -1; 
        StickCP2.Speaker.stop();

        StickCP2.Display.fillScreen(BLACK);
        StickCP2.Display.setCursor(0, 0);
        StickCP2.Display.setTextColor(RED);

        if (currentState == OFF) StickCP2.Display.println("Off");
        else if (currentState == HOME) StickCP2.Display.println("Home");
        else if (currentState == AWAY) StickCP2.Display.println("Away");

        if (mqttClient.connected()) {
        modeMessage.mode = currentState;
        modeMessage.t = millis();
        mqttClient.publish(MODE_TOPIC, (uint8_t*)&modeMessage, sizeof(ModeMessage));
        }
    }
}

void setup() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextSize(3);
    
    WiFi.begin("ssid", "password"); 
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(mqttCallback);

    pinMode(REED_SWITCH, INPUT_PULLUP);
}

void loop() {
    StickCP2.update();

    if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
        connectToMQTT();
    }
    mqttClient.loop();

    // --- NEW: HEARTBEAT LOGIC ---
    if (millis() - lastHeartbeat >= heartbeatInterval) {
        if (mqttClient.connected()) {
        modeMessage.mode = currentState;
        modeMessage.open = (digitalRead(REED_SWITCH) == LOW);
        modeMessage.t = millis();
        mqttClient.publish(HEARTBEAT_TOPIC, (uint8_t*)&modeMessage, sizeof(ModeMessage));
        lastHeartbeat = millis();
        Serial.println("Heartbeat sent.");
        }
    }

    int currentSensorState = digitalRead(REED_SWITCH);

    if (currentSensorState != lastState && (millis() - lastDebounceTime) > debounceDelay) {
        if (currentState != OFF) {
        if (currentState == AWAY && !alarmTripped) {
            alarmTripped = true;
            tripStartTime = millis();
        }
        
        if (!alarmTripped) {
            StickCP2.Display.fillScreen(BLACK);
            StickCP2.Display.setCursor(0, 0);
            StickCP2.Display.setTextColor(WHITE);
            StickCP2.Display.println(currentSensorState == LOW ? "OPEN" : "CLOSED");
        }

        if (mqttClient.connected()) {
            modeMessage.mode = currentState;
            modeMessage.open = (currentSensorState == LOW);
            modeMessage.t = millis();
            mqttClient.publish(SENSOR_TOPIC, (uint8_t*)&modeMessage, sizeof(ModeMessage));
        }
        }
        lastState = currentSensorState;
        lastDebounceTime = millis();
    }

    if (alarmTripped) {
        unsigned long elapsed = millis() - tripStartTime;
        int secondsLeft = (alarmDelay - elapsed) / 1000;

        if (elapsed < alarmDelay) {
        if (secondsLeft != lastSecondsLeft) {
            StickCP2.Display.fillScreen(BLACK); 
            StickCP2.Display.setCursor(10, 30);
            StickCP2.Display.setTextColor(YELLOW);
            StickCP2.Display.printf("DISARM: %d", secondsLeft);
            lastSecondsLeft = secondsLeft;
        }
        } else {
        StickCP2.Display.fillScreen(RED);
        StickCP2.Display.setCursor(10, 30);
        StickCP2.Display.setTextColor(WHITE);
        StickCP2.Display.println("ALARM!");
        StickCP2.Speaker.tone(4000, 100); 
        }
    }

    modeSelect();
}