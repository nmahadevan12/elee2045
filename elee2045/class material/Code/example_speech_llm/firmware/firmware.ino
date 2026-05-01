/*
 * M5StickC Plus 2 — Audio streaming to WebSocket server
 *
 * Arduino IDE Setup:
 *   Board Manager URL: https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
 *   Board: M5StickCPlus2 (m5stack:esp32)
 *   Libraries: M5Unified, WebSockets (Markus Sattler)
 *
 * WiFi credentials are read from ESP32 Preferences (namespace "wifi").
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <Preferences.h>

// ============ SERVER CONFIG ============
#define SERVER_HOST "elee2045sp26.chickenkiller.com"
#define SERVER_PORT 443
#define SERVER_PATH "/example_llm"

// ============ AUDIO CONFIG ============
#define SAMPLE_RATE 16000
#define MIC_BUF_LEN 256  // samples per chunk (~16ms at 16kHz)

// ============ GLOBALS ============
WebSocketsClient ws;
enum State { DISCONNECTED, READY, RECORDING, PROCESSING };
State state = DISCONNECTED;
bool firstChunk = true;

// ============ WIFI FROM PREFERENCES ============
bool connectWiFi() {
    Preferences prefs;
    prefs.begin("wifi", true);
    uint8_t count = prefs.getUChar("count", 0);

    for (uint8_t i = 0; i < count; i++) {
        char key[8];
        snprintf(key, sizeof(key), "ssid%d", i);
        String ssid = prefs.getString(key, "");
        snprintf(key, sizeof(key), "user%d", i);
        String user = prefs.getString(key, "");
        snprintf(key, sizeof(key), "pass%d", i);
        String pass = prefs.getString(key, "");

        if (ssid.isEmpty()) continue;

        M5.Display.printf("Trying: %s\n", ssid.c_str());

        WiFi.mode(WIFI_STA);
        WiFi.disconnect();

        if (user.length() > 0) {
            WiFi.begin(ssid.c_str(), WPA2_AUTH_PEAP, user.c_str(), user.c_str(), pass.c_str());
        } else {
            WiFi.begin(ssid.c_str(), pass.c_str());
        }

        for (int t = 0; t < 10 && WiFi.status() != WL_CONNECTED; t++) {
            delay(1000);
            M5.Display.print(".");
        }
        M5.Display.println();

        if (WiFi.status() == WL_CONNECTED) {
            prefs.end();
            return true;
        }
    }

    prefs.end();
    return false;
}

// ============ DISPLAY HELPERS ============
void showStatus(const char *text, uint16_t color = WHITE) {
    M5.Display.fillRect(0, 0, 240, 18, BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(color, BLACK);
    M5.Display.setCursor(4, 2);
    M5.Display.print(text);
}

void clearBody() {
    M5.Display.fillRect(0, 22, 240, 113, BLACK);
    M5.Display.setCursor(4, 26);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(WHITE, BLACK);
}

// ============ WEBSOCKET HANDLER ============
void onWebSocket(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            state = DISCONNECTED;
            showStatus("No Server", RED);
            break;

        case WStype_CONNECTED:
            state = READY;
            showStatus("Ready", GREEN);
            clearBody();
            M5.Display.print("Press A to record");
            break;

        case WStype_TEXT: {
            String msg((char *)payload);

            if (msg.startsWith("T:")) {
                showStatus("Thinking...", YELLOW);
                clearBody();
                M5.Display.setTextColor(CYAN, BLACK);
                M5.Display.print("You: ");
                M5.Display.setTextColor(WHITE, BLACK);
                M5.Display.println(msg.substring(2));
                M5.Display.println();
                firstChunk = true;

            } else if (msg.startsWith("R:")) {
                if (firstChunk) {
                    showStatus("Response", CYAN);
                    clearBody();
                    firstChunk = false;
                }
                M5.Display.print(msg.substring(2));

            } else if (msg == "D") {
                state = READY;
                showStatus("Ready", GREEN);
            }
            break;
        }

        default:
            break;
    }
}

// ============ SETUP ============
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);

    showStatus("WiFi...", YELLOW);
    clearBody();

    if (!connectWiFi()) {
        showStatus("No WiFi!", RED);
        while (true) delay(1000);
    }

    M5.Display.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    delay(1000);

    // Mic and speaker share I2S — disable speaker first
    M5.Speaker.end();
    M5.Mic.begin();

    // Connect WebSocket over TLS
    ws.beginSSL(SERVER_HOST, SERVER_PORT, SERVER_PATH);
    ws.onEvent(onWebSocket);
    ws.setReconnectInterval(3000);
}

// ============ LOOP ============
void loop() {
    M5.update();
    ws.loop();

    // Button A toggles recording
    if (M5.BtnA.wasPressed()) {
        if (state == READY) {
            state = RECORDING;
            firstChunk = true;
            ws.sendTXT("start");
            showStatus("Recording", RED);
            clearBody();
            M5.Display.print("Listening...");

        } else if (state == RECORDING) {
            state = PROCESSING;
            ws.sendTXT("stop");
            showStatus("Processing", YELLOW);
        }
    }

    // Stream mic audio while recording
    if (state == RECORDING) {
        int16_t buf[MIC_BUF_LEN];
        if (M5.Mic.record(buf, MIC_BUF_LEN, SAMPLE_RATE)) {
            ws.sendBIN((uint8_t *)buf, sizeof(buf));
        }
    }
}
