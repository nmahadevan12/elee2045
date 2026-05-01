// ── Libraries ────────────────────────────────────────────────────────────────
#include <M5Unified.h>          // M5Stick/M5Core hardware abstraction
#include <WiFi.h>               // WiFi connectivity
#include <WiFiClientSecure.h>   // HTTPS support
#include <HTTPClient.h>         // HTTP requests
#include <ArduinoJson.h>        // JSON parsing
#include <vector>               // Dynamic arrays (PCM + WAV)
#include <freertos/FreeRTOS.h>  // RTOS core
#include <freertos/task.h>      // Task creation
#include <freertos/semphr.h>    // Mutexes (thread safety)


// ── Configuration ────────────────────────────────────────────────────────────
const char* WIFI_SSID = "cuscuta";
const char* WIFI_PASS = "SSID";
const char* BASE_URL = "https://0518-73-7-194-238.ngrok-free.app"; // Backend URL
const char* API_KEY = "device-key"; // Must match Flask backend

const char* NGROK_SKIP_HEADER = "true"; // Bypass ngrok warning page
const unsigned long POLL_MS = 60000;    // Poll todos every 60s
const unsigned long HOLD_MS = 700;      // Button hold threshold (ms)

const int MAX_TODOS = 24;               // Max todos stored locally
const int MAX_TODO_LEN = 96;            // Max length of each todo string

// Audio recording config
const int SAMPLE_RATE = 16000;          // 16kHz audio
const int MAX_RECORD_SECONDS = 6;       // Max recording length
const int CHUNK_SIZE = 256;             // Samples per read


// ── Data Structures ──────────────────────────────────────────────────────────
struct TodoItem {
  int id;
  bool done;
  char text[MAX_TODO_LEN];
};

// Local todo storage
TodoItem todos[MAX_TODOS];
int todoCount = 0;
int todoIndex = 0;

// Timing + button tracking
unsigned long lastPoll = 0;
unsigned long bPressStart = 0;
bool bWasPressed = false;


// ── Recording State (shared across cores → volatile) ─────────────────────────
volatile bool recording = false;
volatile bool recordingDone = false;

// Mutex to protect PCM buffer between tasks
SemaphoreHandle_t pcmMutex;

// Audio buffer (raw PCM samples)
std::vector<int16_t> pcm;

// Handle for microphone task (runs on Core 0)
TaskHandle_t micTaskHandle = NULL;


// ── UI Helpers ───────────────────────────────────────────────────────────────

// Display a message on screen + serial
void drawStatus(const String& msg, bool persist = false) {
  Serial.println(msg);

  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(4, 4);
  M5.Display.setTextSize(2);
  M5.Display.println(msg);

  // Small delay unless persistent (used for recording/upload states)
  if (!persist) delay(500);
}

// Draw current todo item
void drawTodo() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(4, 4);
  M5.Display.setTextSize(2);

  if (todoCount == 0) {
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.println("No Todos");
    return;
  }

  // Show index position
  M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Display.printf("[%d/%d]\n", todoIndex + 1, todoCount);

  // Gray out completed tasks
  M5.Display.setTextColor(
    todos[todoIndex].done ? TFT_DARKGREY : TFT_WHITE, TFT_BLACK
  );

  M5.Display.println(todos[todoIndex].text);
}


// ── WiFi Handling ────────────────────────────────────────────────────────────

// Ensure WiFi connection (retry if disconnected)
bool wifiEnsure() {
  if (WiFi.status() == WL_CONNECTED) return true;

  drawStatus("WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();

  // Wait up to 15 seconds
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    drawStatus("WiFi Failed");
    return false;
  }

  drawStatus("WiFi OK");
  return true;
}


// ── JSON Parsing ─────────────────────────────────────────────────────────────

// Parse JSON array → populate todos[]
bool parseTodos(const String& body) {
  DynamicJsonDocument doc(8192);

  DeserializationError err = deserializeJson(doc, body);
  if (err || !doc.is<JsonArray>()) return false;

  todoCount = 0;

  for (JsonVariant v : doc.as<JsonArray>()) {
    if (todoCount >= MAX_TODOS) break;

    todos[todoCount].id = v["id"] | 0;
    todos[todoCount].done = v["done"] | false;

    // Safe string copy
    const char* text = v["text"] | "";
    strncpy(todos[todoCount].text, text, MAX_TODO_LEN - 1);
    todos[todoCount].text[MAX_TODO_LEN - 1] = '\0';

    todoCount++;
  }

  // Reset index if out of bounds
  if (todoIndex >= todoCount) todoIndex = 0;

  return true;
}


// ── Backend API Calls ────────────────────────────────────────────────────────

// Fetch todos from server
void fetchTodos() {
  if (!wifiEnsure()) return;

  WiFiClientSecure client;
  client.setInsecure(); // Skip SSL validation (ngrok)

  HTTPClient http;

  if (!http.begin(client, String(BASE_URL) + "/api/todos")) {
    drawStatus("Server URL Err");
    return;
  }

  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("ngrok-skip-browser-warning", NGROK_SKIP_HEADER);

  int code = http.GET();
  Serial.printf("GET /api/todos -> %d\n", code);

  if (code == 200) {
    String body = http.getString();

    if (parseTodos(body)) {
      drawStatus("Todos Updated");
      drawTodo();
    } else {
      drawStatus("JSON Error");
    }

  } else if (code == 401) {
    drawStatus("Auth Failed");
  } else {
    drawStatus("Server " + String(code));
  }

  http.end();
}


// Mark a todo as done (POST)
void markDone(int todoId) {
  if (!wifiEnsure()) return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  if (!http.begin(client, String(BASE_URL) + "/api/todos/" + String(todoId) + "/done")) {
    drawStatus("Server URL Err");
    return;
  }

  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("ngrok-skip-browser-warning", NGROK_SKIP_HEADER);

  int code = http.POST("");
  Serial.printf("POST /api/todos/%d/done -> %d\n", todoId, code);

  drawStatus(code == 200 ? "Done" : "Done Fail");

  http.end();

  // Refresh list after update
  fetchTodos();
}


// Delete a todo (DELETE request)
void deleteTodo(int todoId) {
  if (!wifiEnsure()) return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  if (!http.begin(client, String(BASE_URL) + "/api/todos/" + String(todoId))) {
    drawStatus("Server URL Err");
    return;
  }

  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("ngrok-skip-browser-warning", NGROK_SKIP_HEADER);

  int code = http.sendRequest("DELETE");
  Serial.printf("DELETE /api/todos/%d -> %d\n", todoId, code);

  drawStatus(code == 200 ? "Deleted" : "Delete Fail");

  http.end();
  fetchTodos();
}


// ── Mic Task (Core 0) ────────────────────────────────────────────────────────
// Runs separately from main loop to avoid blocking UI/network
void micTask(void* pvParams) {
  Serial.println("Mic task started");

  vTaskDelay(pdMS_TO_TICKS(150)); // Let DMA stabilize

  int16_t buf[CHUNK_SIZE];

  while (recording) {
    // Read chunk from mic
    if (M5.Mic.record(buf, CHUNK_SIZE, SAMPLE_RATE)) {

      // Lock buffer before writing
      if (xSemaphoreTake(pcmMutex, pdMS_TO_TICKS(10)) == pdTRUE) {

        pcm.insert(pcm.end(), buf, buf + CHUNK_SIZE);

        // Stop if max duration reached
        bool full = (int)pcm.size() >= SAMPLE_RATE * MAX_RECORD_SECONDS;

        xSemaphoreGive(pcmMutex);

        if (full) {
          recording = false;
          break;
        }
      }
    }

    taskYIELD(); // Prevent watchdog reset
  }

  Serial.println("Mic task ending");

  recordingDone = true; // Signal main loop
  micTaskHandle = NULL;

  vTaskDelete(NULL); // Self-delete
}


// ── Recording Control ────────────────────────────────────────────────────────

// Start recording (spawns mic task)
void startRecording() {
  if (recording) return;

  // Clear buffer safely
  xSemaphoreTake(pcmMutex, portMAX_DELAY);
  pcm.clear();
  pcm.reserve(SAMPLE_RATE * MAX_RECORD_SECONDS);
  xSemaphoreGive(pcmMutex);

  // Initialize mic
  if (!M5.Mic.begin()) {
    drawStatus("Mic Error");
    return;
  }

  recording = true;
  recordingDone = false;

  // Run mic task on Core 0
  BaseType_t res = xTaskCreatePinnedToCore(
    micTask, "micTask",
    4096, NULL, 1,
    &micTaskHandle,
    0
  );

  if (res != pdPASS) {
    recording = false;
    M5.Mic.end();
    drawStatus("Task Error");
    return;
  }

  drawStatus("RECORDING", true);
}


// Cancel recording (no upload)
void cancelRecording() {
  if (!recording && micTaskHandle == NULL) return;

  recording = false;

  // Wait for task cleanup
  unsigned long wait = millis();
  while (micTaskHandle != NULL && millis() - wait < 1000) delay(10);

  M5.Mic.end();

  // Clear buffer
  xSemaphoreTake(pcmMutex, portMAX_DELAY);
  pcm.clear();
  xSemaphoreGive(pcmMutex);

  drawStatus("Cancelled");
  drawTodo();
}


// Convert PCM → WAV format (adds header)
std::vector<uint8_t> wavFromPcm() {
  const uint32_t dataSize = pcm.size() * sizeof(int16_t);
  const uint32_t fileSize = 36 + dataSize;

  std::vector<uint8_t> out(44 + dataSize);
  uint8_t* p = out.data();

  // Standard WAV header
  memcpy(p, "RIFF", 4); p += 4;
  memcpy(p, &fileSize, 4); p += 4;
  memcpy(p, "WAVEfmt ", 8); p += 8;

  uint32_t subchunk1 = 16; memcpy(p, &subchunk1, 4); p += 4;
  uint16_t fmt = 1; memcpy(p, &fmt, 2); p += 2;

  uint16_t channels = 1; memcpy(p, &channels, 2); p += 2;
  uint32_t sr = SAMPLE_RATE; memcpy(p, &sr, 4); p += 4;

  uint32_t byteRate = SAMPLE_RATE * 2; memcpy(p, &byteRate, 4); p += 4;
  uint16_t blockAlign = 2; memcpy(p, &blockAlign, 2); p += 2;
  uint16_t bits = 16; memcpy(p, &bits, 2); p += 2;

  memcpy(p, "data", 4); p += 4;
  memcpy(p, &dataSize, 4); p += 4;

  // Copy audio data
  memcpy(p, pcm.data(), dataSize);

  return out;
}


// Upload WAV file to backend
bool uploadWav(const std::vector<uint8_t>& wav) {
  if (!wifiEnsure()) return false;

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(30);

  HTTPClient http;

  if (!http.begin(client, String(BASE_URL) + "/api/audio")) {
    Serial.println("HTTP begin fail");
    return false;
  }

  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("ngrok-skip-browser-warning", NGROK_SKIP_HEADER);
  http.addHeader("Content-Type", "multipart/form-data; boundary=----m5stickboundary");

  // Build multipart body (audio upload)
  const String boundary = "----m5stickboundary";

  const String part1 =
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"device_id\"\r\n\r\n"
      "m5stick\r\n";

  const String part2Header =
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"audio\"; filename=\"note.wav\"\r\n"
      "Content-Type: audio/wav\r\n\r\n";

  const String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t totalLen = part1.length() + part2Header.length() + wav.size() + tail.length();

  std::vector<uint8_t> body;
  body.reserve(totalLen);

  // Append parts
  for (char c : part1) body.push_back((uint8_t)c);
  for (char c : part2Header) body.push_back((uint8_t)c);
  for (uint8_t b : wav) body.push_back(b);
  for (char c : tail) body.push_back((uint8_t)c);

  Serial.printf("Uploading %u bytes\n", (unsigned)body.size());

  int code = http.POST(body.data(), body.size());

  Serial.printf("POST /api/audio -> %d\n", code);
  Serial.println("Response: " + http.getString());

  http.end();

  return (code == 200 || code == 201);
}


// Stop recording → convert → upload
void stopAndUpload() {
  if (!recording && micTaskHandle == NULL) return;

  recording = false;

  // Wait for mic task
  unsigned long wait = millis();
  while (micTaskHandle != NULL && millis() - wait < 1000) delay(10);

  M5.Mic.end();

  // Check if enough audio was recorded
  xSemaphoreTake(pcmMutex, portMAX_DELAY);
  int samples = pcm.size();
  xSemaphoreGive(pcmMutex);

  if (samples < 500) {
    xSemaphoreTake(pcmMutex, portMAX_DELAY);
    pcm.clear();
    xSemaphoreGive(pcmMutex);

    drawStatus("Too Short");
    drawTodo();
    return;
  }

  drawStatus("Uploading...", true);

  // Convert + free memory early
  xSemaphoreTake(pcmMutex, portMAX_DELAY);
  std::vector<uint8_t> wav = wavFromPcm();
  pcm.clear();
  xSemaphoreGive(pcmMutex);

  bool ok = uploadWav(wav);

  drawStatus(ok ? "Success" : "Upload Fail");

  fetchTodos();
}


// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(200);

  pcmMutex = xSemaphoreCreateMutex(); // Init mutex

  // Configure M5 device
  auto cfg = M5.config();
  cfg.internal_mic = true;
  M5.begin(cfg);

  // Configure mic
  auto micCfg = M5.Mic.config();
  micCfg.sample_rate = SAMPLE_RATE;
  micCfg.stereo = false;
  M5.Mic.config(micCfg);
  M5.Mic.end();

  // Display setup
  M5.Display.wakeup();
  M5.Display.setBrightness(200);
  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

  drawStatus("Booting...");

  wifiEnsure();
  fetchTodos();
}


// ── Main Loop ────────────────────────────────────────────────────────────────
void loop() {
  M5.update(); // Update button states

  // If recording finished naturally → upload
  if (recordingDone && micTaskHandle == NULL) {
    recordingDone = false;
    stopAndUpload();
    return;
  }

  // Button A → start/stop recording
  if (M5.BtnA.wasPressed()) {
    if (recording || micTaskHandle != NULL) stopAndUpload();
    else startRecording();
  }

  // Power button → cancel recording
  if ((recording || micTaskHandle != NULL) && M5.BtnPWR.wasPressed()) {
    cancelRecording();
  }

  // Button B → navigation / mark done
  if (M5.BtnB.wasPressed()) {
    bPressStart = millis();
    bWasPressed = true;
  }

  if (bWasPressed && M5.BtnB.wasReleased()) {
    unsigned long held = millis() - bPressStart;
    bWasPressed = false;

    if (recording || todoCount == 0) return;

    if (held >= HOLD_MS) {
      markDone(todos[todoIndex].id); // Long press → done
    } else {
      todoIndex = (todoIndex + 1) % todoCount; // Short press → next
      drawTodo();
    }
  }

  // Periodically refresh todos
  if (!recording && millis() - lastPoll > POLL_MS) {
    lastPoll = millis();
    fetchTodos();
  }

  delay(10); // Small loop delay
}