#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <Wire.h>
#include "MAX30105.h"

// --- WiFi and Firebase Credentials ---
#define WIFI_SSID "Xiaomi 11i"
#define WIFI_PASSWORD "910886206"
#define FIREBASE_HOST "FILLYOURFIREBASEHOST"
#define FIREBASE_AUTH "FILLYOURFIREBASEAUTH"

FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

MAX30105 particleSensor;

const uint8_t SDA_PIN = 4;
const uint8_t SCL_PIN = 5;

const uint8_t RGB_R = 14;
const uint8_t RGB_G = 12;
const uint8_t RGB_B = 13;

const long FINGER_THRESHOLD = 30000;
const long FINGER_DETECTING_THRESHOLD = 10000;

const long BEAT_DROP_THRESHOLD = 500;
const long MIN_BEAT_INTERVAL_MS = 333;
const long MAX_BEAT_INTERVAL_MS = 1500;

long irValue = 0;
long redValue = 0;

#define IR_BUFFER_SIZE 25
long irBuffer[IR_BUFFER_SIZE];
int irBufferIdx = 0;
bool irBufferFull = false;

long irPeak = 0;
long irValley = 0;
bool lookingForValley = false;
unsigned long lastBeatTime = 0;

float bpm = 0.0;
float avgBpm = 0.0;
float lastValidBpm = 0.0;

byte rates[8];
byte rateSpot = 0;
byte validRateCount = 0;

unsigned long lastPrint = 0;

// ── Multi-session tracking ────────────────────────────────────
unsigned long sessionStartTime = 0;
bool sessionStarted   = false;
int currentRound      = 0;
unsigned long roundStartTime = 0;

#define MAX_ROUNDS 10
float roundAvg[MAX_ROUNDS];
int roundsFilled = 0;
bool roundResultPrinted = false;

void setRGB(bool r, bool g, bool b) {
  digitalWrite(RGB_R, r ? HIGH : LOW);
  digitalWrite(RGB_G, g ? HIGH : LOW);
  digitalWrite(RGB_B, b ? HIGH : LOW);
}
void colorOff()    { setRGB(0,0,0); }
void colorRed()    { setRGB(1,0,0); }
void colorGreen()  { setRGB(0,1,0); }
void colorBlue()   { setRGB(0,0,1); }
void colorYellow() { setRGB(1,1,0); }
void colorCyan()   { setRGB(0,1,1); }
void colorPurple() { setRGB(1,0,1); }
void colorWhite()  { setRGB(1,1,1); }

void pushToFirebase(String path, float value) {
  if (Firebase.ready()) {
    Firebase.setFloat(firebaseData, path, value);
  }
}

void resetBpmState() {
  irPeak = 0;
  irValley = 0;
  lookingForValley = false;
  lastBeatTime = 0;
  bpm = 0.0;
  avgBpm = 0.0;
  rateSpot = 0;
  validRateCount = 0;
  irBufferIdx = 0;
  irBufferFull = false;
  for (byte i = 0; i < 8; i++) rates[i] = 0;
}

void fullReset() {
  resetBpmState();
  lastValidBpm = 0.0;
  sessionStarted = false;
  currentRound = 0;
  roundsFilled = 0;
  roundResultPrinted = false;
  for (int i = 0; i < MAX_ROUNDS; i++) roundAvg[i] = 0.0;
}

void printRoundResult(int round, float avg) {
  Serial.println();
  Serial.println("==========================================");
  Serial.print  ("  ROUND "); Serial.print(round);
  Serial.println(" — 30-SEC AVERAGE BPM: " + String(avg, 1));
  Serial.println("==========================================");
  pushToFirebase("/sessions/round_" + String(round), avg);
}

void printCombinedResult(int r1, int r2) {
  float combined = (roundAvg[r1 - 1] + roundAvg[r2 - 1]) / 2.0;
  Serial.println("------------------------------------------");
  Serial.print  ("  ROUNDS "); Serial.print(r1);
  Serial.print  (" + ");       Serial.print(r2);
  Serial.print  (" COMBINED AVG BPM: ");
  Serial.println(combined, 1);
  Serial.println("------------------------------------------");
  pushToFirebase("/sessions/combined_avg", combined);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RGB_R, OUTPUT);
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);
  colorOff();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found.");
    while (1) { colorPurple(); delay(500); colorOff(); delay(500); }
  }

  particleSensor.setup(0x1F, 4, 2, 100, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);

  Serial.println("Ready. Place finger to begin.");
}

void loop() {
  irValue  = particleSensor.getIR();
  redValue = particleSensor.getRed();

  bool fingerFull      = (irValue > FINGER_THRESHOLD);
  bool fingerDetecting = (!fingerFull && irValue > FINGER_DETECTING_THRESHOLD);

  if (fingerFull && !sessionStarted) {
    sessionStarted     = true;
    currentRound       = 1;
    roundStartTime     = millis();
    roundResultPrinted = false;
    resetBpmState();
    lastValidBpm = 0.0;
    Serial.println("=== Round 1 started. Keep finger still. ===");
  }

  if (!fingerFull && !fingerDetecting && sessionStarted) {
    Serial.println("! Finger removed — session stopped. Place finger to restart.");
    fullReset();
  }

  if (fingerFull) {
    irBuffer[irBufferIdx++] = irValue;
    if (irBufferIdx >= IR_BUFFER_SIZE) { irBufferIdx = 0; irBufferFull = true; }

    unsigned long now = millis();
    if (!lookingForValley) {
      if (irValue > irPeak) irPeak = irValue;
      if (irPeak > 0 && (irPeak - irValue) > BEAT_DROP_THRESHOLD) {
        lookingForValley = true;
        irValley = irValue;
        if (lastBeatTime > 0) {
          long delta = now - lastBeatTime;
          if (delta >= MIN_BEAT_INTERVAL_MS && delta <= MAX_BEAT_INTERVAL_MS) {
            lastValidBpm = 60000.0 / (float)delta;
            rates[rateSpot++] = (byte)lastValidBpm;
            rateSpot %= 8;
            if (validRateCount < 8) validRateCount++;
            long sum = 0;
            for (byte i = 0; i < validRateCount; i++) sum += rates[i];
            avgBpm = (float)sum / validRateCount;
            Firebase.setFloat(firebaseData, "/live/bpm", lastValidBpm);
          }
        }
        lastBeatTime = now;
      }
    } else {
      if (irValue < irValley) irValley = irValue;
      if ((irValue - irValley) > BEAT_DROP_THRESHOLD / 2) { lookingForValley = false; irPeak = irValue; }
    }

    unsigned long roundElapsed = millis() - roundStartTime;
    if (roundElapsed >= 30000 && !roundResultPrinted) {
      roundResultPrinted = true;
      float thisAvg = (avgBpm > 0) ? avgBpm : lastValidBpm;
      if (roundsFilled < MAX_ROUNDS) roundAvg[roundsFilled++] = thisAvg;
      printRoundResult(currentRound, thisAvg);
      if (currentRound % 2 == 0) printCombinedResult(currentRound - 1, currentRound);
      currentRound++;
      roundStartTime = millis();
      roundResultPrinted = false;
      resetBpmState();
      Serial.print("=== Round "); Serial.print(currentRound); Serial.println(" started. ===");
    }
  }

  // ── SERIAL MONITOR OUTPUT (Matched to Image) ──
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    Serial.print("IR=");      Serial.print(irValue);
    Serial.print(" | RED=");   Serial.print(redValue);
    Serial.print(" | Finger=");
    if (fingerFull)           Serial.print("YES");
    else if (fingerDetecting)  Serial.print("DETECTING");
    else                      Serial.print("NO");

    Serial.print(" | BPM=");
    if (!fingerFull && !fingerDetecting) Serial.print("--");
    else if (lastValidBpm == 0)          Serial.print("wait...");
    else                                 Serial.print(lastValidBpm, 1);

    Serial.print(" | AVG=");
    if (avgBpm > 0) Serial.print(avgBpm, 1);
    else            Serial.print("--");

    if (sessionStarted && fingerFull) {
      unsigned long roundElapsed = millis() - roundStartTime;
      int secsLeft = (int)((30000 - roundElapsed) / 1000) + 1;
      if (secsLeft > 0) {
        Serial.print(" | R"); Serial.print(currentRound);
        Serial.print(": "); Serial.print(secsLeft); Serial.print("s left");
      }
    }
    Serial.println();
  }

  // RGB Indicators
  if (fingerDetecting) colorYellow();
  else if (!fingerFull) colorWhite();
  else if (lastValidBpm == 0) colorCyan();
  else if (avgBpm < 60.0) colorBlue();
  else if (avgBpm <= 100.0) colorGreen();
  else colorRed();

  delay(10);
}
