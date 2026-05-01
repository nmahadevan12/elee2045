#include <M5StickCPlus2.h>

// --- STATE MACHINE DEFINITIONS ---
enum GameState {
  IDLE,
  WAIT,
  REACTION,
  RESULT,
  FOUL
};

GameState currentState = IDLE;

// --- TIMING VARIABLES ---
unsigned long waitStartTime = 0;
unsigned long randomDelay = 0;
unsigned long reactionStartTime = 0;
long finalReactionTime = 0;

// --- EXTENSION 1: STATISTICS VARIABLES ---
// We will track the last 5 attempts to calculate an average
#define MAX_HISTORY 5
long history[MAX_HISTORY];
int historyCount = 0;
int historyIndex = 0;

void setup() {
  // 1. Initialize M5StickC Plus 2 Config
  auto cfg = M5.config();
  M5.begin(cfg);

  // 2. Screen Setup
  M5.Lcd.setRotation(3); // Landscape mode (USB to the right)
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(BLACK);

  // 3. Hardware Randomness (Critical Requirement)
  // Use ESP32's true hardware random number generator to seed
  randomSeed(esp_random());

  // 4. Initial Screen
  drawIdleScreen();
}

void loop() {
  M5.update(); // MUST be called to read buttons

  switch (currentState) {
    // --- STATE: IDLE ---
    case IDLE:
      if (M5.BtnA.wasPressed()) {
        startGame();
      }
      break;

    // --- STATE: WAIT ---
    case WAIT:
      // FAIL CONDITION: User pressed too early
      if (M5.BtnA.wasPressed()) {
        triggerFoul();
      }
      // SUCCESS CONDITION: Timer finished
      else if (millis() - waitStartTime >= randomDelay) {
        triggerReaction();
      }
      break;

    // --- STATE: REACTION ---
    case REACTION:
      if (M5.BtnA.wasPressed()) {
        finalReactionTime = millis() - reactionStartTime;
        saveStatistic(finalReactionTime); // Save for Extension 1
        showResult();
      }
      break;

    // --- STATE: RESULT ---
    case RESULT:
      // Press button to restart
      if (M5.BtnA.wasPressed()) {
        drawIdleScreen();
      }
      break;

    // --- STATE: FOUL ---
    case FOUL:
      // Press button to acknowledge foul and restart
      if (M5.BtnA.wasPressed()) {
        drawIdleScreen();
      }
      break;
  }
}

// --- HELPER FUNCTIONS ---

void drawIdleScreen() {
  currentState = IDLE;
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextDatum(middle_center);
  M5.Lcd.drawString("Press M5", M5.Lcd.width() / 2, M5.Lcd.height() / 2 - 20);
  M5.Lcd.drawString("to Start", M5.Lcd.width() / 2, M5.Lcd.height() / 2 + 10);
}

void startGame() {
  currentState = WAIT;
  M5.Lcd.fillScreen(RED);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextDatum(middle_center);
  M5.Lcd.drawString("WAIT...", M5.Lcd.width() / 2, M5.Lcd.height() / 2);

  // Set random delay between 2000ms and 5000ms
  waitStartTime = millis();
  randomDelay = random(2000, 5001);
}

void triggerFoul() {
  currentState = FOUL;
  M5.Lcd.fillScreen(ORANGE);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setTextDatum(middle_center);
  M5.Lcd.drawString("TOO EARLY!", M5.Lcd.width() / 2, M5.Lcd.height() / 2);
  delay(1000); // Penalty delay so they can't spam click
}

void triggerReaction() {
  currentState = REACTION;
  reactionStartTime = millis();
  M5.Lcd.fillScreen(GREEN); // Visual stimulus
}

// --- EXTENSION LOGIC ---

void saveStatistic(long time) {
  history[historyIndex] = time;
  historyIndex = (historyIndex + 1) % MAX_HISTORY; // Wrap around
  if (historyCount < MAX_HISTORY) historyCount++;
}

long calculateAverage() {
  if (historyCount == 0) return 0;
  long sum = 0;
  for (int i = 0; i < historyCount; i++) {
    sum += history[i];
  }
  return sum / historyCount;
}

void showResult() {
  currentState = RESULT;
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextDatum(top_left);

  // Standard Output
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.printf("Time: %ld ms", finalReactionTime);

  // Extension 1: Display Average
  long avg = calculateAverage();
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.printf("Avg: %ld ms", avg);

  // Extension 2: Visual Performance Bar
  // Draw a bar at the bottom.
  // Map 0-1000ms to the screen width. Green=Fast, Red=Slow
  int barWidth = map(constrain(finalReactionTime, 0, 1000), 0, 1000, 0, M5.Lcd.width());
  uint16_t barColor = (finalReactionTime < 250) ? GREEN : (finalReactionTime < 400) ? YELLOW : RED;
  M5.Lcd.fillRect(0, M5.Lcd.height() - 20, barWidth, 20, barColor);

  // Retry text
  M5.Lcd.setTextDatum(bottom_center);
  M5.Lcd.drawString("Press to Retry", M5.Lcd.width() / 2, M5.Lcd.height() - 30);
}