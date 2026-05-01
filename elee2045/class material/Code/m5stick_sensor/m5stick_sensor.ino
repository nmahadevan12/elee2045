#include "M5Unified.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "Preferences.h"
#include "WifiFunctions.h" 

Preferences preferences;
M5Canvas canvas(&M5.Display);

WifiCredential wifiCredentials[MAX_WIFI_NETWORKS];
int wifiCount = 0;

unsigned long lastPostTime = 0;
const unsigned long postInterval = 3000; // 3 seconds

// Your Cloudflare Tunnel endpoint
const char* serverUrl = "https://just-beautifully-avenue-cholesterol.trycloudflare.com/report";

// Helper to draw to the screen without flickering
void updateDisplay(const char* status, float noise = -1.0) {
    canvas.fillSprite(TFT_BLACK);
    
    // Header
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_ORANGE);
    canvas.setCursor(5, 5);
    canvas.println("Noise Monitor");

    // Status Text
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(5, 30);
    canvas.println(status);

    // Network Info
    if (WiFi.status() == WL_CONNECTED) {
        canvas.setTextSize(1);
        canvas.setCursor(5, 60);
        canvas.print("IP: ");
        canvas.println(WiFi.localIP().toString());
        canvas.setCursor(5, 75);
        canvas.print("MAC: ");
        canvas.println(WiFi.macAddress());
    }

    // Latest Noise Reading
    if (noise >= 0.0) {
        canvas.setTextSize(3);
        canvas.setTextColor(TFT_GREEN);
        canvas.setCursor(5, 95);
        canvas.printf("%.1f dB\n", noise);
    }

    // Push the sprite to the physical display
    canvas.pushSprite(0, 0);
}

void sendNoiseData(float noiseLevel) {
    WiFiClientSecure secureClient;
    secureClient.setInsecure(); // Ignore SSL certificate validation for this test
    HTTPClient http;

    if (http.begin(secureClient, serverUrl)) {
        http.addHeader("Content-Type", "application/json");
        
        String mac = WiFi.macAddress();
        // Construct JSON Payload
        String payload = "{\"mac_address\":\"" + mac + "\",\"noise_level\":" + String(noiseLevel, 1) + "}";
        
        int httpResponseCode = http.POST(payload);

        if (httpResponseCode > 0) {
            Serial.printf("HTTP Response code: %d\n", httpResponseCode);
        } else {
            Serial.printf("Error code: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    } else {
        Serial.println("Unable to connect to server");
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);
    
    // Initialize Microphone
    M5.Mic.begin();
    
    // Setup Display & Canvas for M5StickCPlus2
    M5.Display.setRotation(1); // Landscape mode
    canvas.createSprite(M5.Display.width(), M5.Display.height());

    updateDisplay("Loading Config...");

    // Load WiFi credentials from Preferences
    preferences.begin("wifi", true);
    wifiCount = preferences.getUChar("count", 0);
    char key[10];
    for(int i = 0; i < wifiCount && i < MAX_WIFI_NETWORKS; i++){
        sprintf(key, "ssid%d", i); preferences.getString(key, wifiCredentials[i].ssid, 50);
        sprintf(key, "user%d", i); preferences.getString(key, wifiCredentials[i].username, 50);
        sprintf(key, "pass%d", i); preferences.getString(key, wifiCredentials[i].password, 50);
    }
    preferences.end();
}

void loop() {
    M5.update(); // Update M5Unified internal state (buttons, etc)

    // Reconnect to WiFi if disconnected
    if(WiFi.status() != WL_CONNECTED) {
        updateDisplay("Connecting WiFi...");
        if (connectToWifi(wifiCredentials, wifiCount)) {
            updateDisplay("WiFi Connected!");
        } else {
            updateDisplay("WiFi Failed");
            M5.delay(2000); // Wait before retrying
            return;
        }
    }

    // Every 3 seconds, record mic and send data
    if (millis() - lastPostTime >= postInterval) {
        lastPostTime = millis();

        updateDisplay("Recording...", -1.0);
        
        const size_t num_samples = 1024;
        int16_t rec_data[num_samples];
        float noiseLevel = 40.0; // Default quiet fallback

        // Start recording 1024 samples at 16kHz
        if (M5.Mic.record(rec_data, num_samples, 16000)) {
            // Wait for recording to finish
            while (M5.Mic.isRecording()) {
                M5.update();
                delay(1);
            }
            
            // Calculate Root Mean Square (RMS) of the audio signal
            double sq_sum = 0;
            for (size_t i = 0; i < num_samples; i++) {
                sq_sum += rec_data[i] * rec_data[i];
            }
            float rms = sqrt(sq_sum / num_samples);
            
            // Convert RMS to a pseudo-dB value
            // Offset by ~20 to align standard quiet room to ~35-45dB
            if (rms > 0.1) {
                noiseLevel = 20.0 * log10(rms) + 20.0;
            } else {
                noiseLevel = 35.0; // Absolute noise floor
            }
        }

        updateDisplay("Sending data...", noiseLevel);
        
        // Send the POST payload
        sendNoiseData(noiseLevel);
        
        updateDisplay("Waiting...", noiseLevel);
    }
    
    M5.delay(10); // Small delay to yield to background tasks
}