# ❤️ IoT-Based Heart Rate Monitoring System

An Arduino + Firebase IoT system that reads real-time heart rate data using the MAX30102 pulse oximetry sensor, streams BPM readings to Firebase Realtime Database, and displays live analytics on a web dashboard — complete with multi-session round tracking, BPM classification, PDF/CSV export, and BMI calculation.

---

## 📁 Project Structure

```
IoT-Based-Heart-Rate-Monitoring-System-main/
│
├── heart_rate_monitor_aurdino.ino     # Arduino firmware — sensor reading, BPM logic, Firebase push
├── index.html                         # Web dashboard — live BPM display, chart, history, export
├── Abstract.pdf                       # Project abstract document
├── Project_Report.docx                # Full project report
├── Heart_Rate_Monitoring_Requirements.docx   # System requirements document
├── LAYERS_OF_INTERNET_OF_THINGS.docx  # IoT architecture reference document
└── Circuit Design.jpeg                # Hardware wiring diagram
```

---

## 📦 Hardware & Libraries Used

### Hardware Components

| Component | Purpose |
|---|---|
| ESP8266 (NodeMCU) | Wi-Fi-enabled microcontroller |
| MAX30102 Sensor | Infrared pulse oximetry & heart rate sensing |
| RGB LED | Visual BPM status indicator (color-coded feedback) |
| Connecting wires | I²C wiring between sensor and ESP8266 |

### Arduino Libraries

| Library | Purpose |
|---|---|
| `ESP8266WiFi.h` | Wi-Fi connection management on ESP8266 |
| `FirebaseESP8266.h` | Firebase Realtime Database read/write |
| `Wire.h` | I²C communication with the MAX30102 sensor |
| `MAX30105.h` | Sensor driver — IR/Red LED control and raw data reading |

### Web Dashboard (index.html)

| Library | Purpose |
|---|---|
| Chart.js 4.4.2 | Live BPM line chart rendering |
| jsPDF 2.5.1 | PDF export of session history |
| Firebase JS SDK 9.22.2 | Real-time database listener in the browser |
| Google Fonts (Space Grotesk + JetBrains Mono) | Dashboard typography |

---

## 🗂️ File Descriptions

### `heart_rate_monitor_aurdino.ino`
The Arduino firmware that runs on the ESP8266. Key responsibilities:

- Connects to Wi-Fi and initialises Firebase using `FirebaseESP8266`
- Reads raw IR and Red light values from the MAX30102 over I²C
- Detects heartbeats using a peak-and-valley algorithm with configurable thresholds (`BEAT_DROP_THRESHOLD`, `MIN/MAX_BEAT_INTERVAL_MS`)
- Calculates instantaneous BPM and an 8-sample rolling average (`avgBpm`)
- Runs a **multi-round session system** — each round lasts 30 seconds, stores the average BPM, and optionally prints a combined average for every two consecutive rounds (up to 10 rounds)
- Pushes live BPM to `/live/bpm` and per-round averages to `/sessions/round_N` in Firebase
- Drives an **RGB LED** as a visual indicator: White (no finger), Yellow (detecting), Cyan (calibrating), Green (normal BPM 60–100), Blue (low), Red (high)
- Outputs structured serial monitor logs every second (IR, RED, Finger state, BPM, AVG, time remaining in round)

### `index.html`
A single-file web dashboard. Key responsibilities:

- **Dual data source mode**: switches between live Firebase Realtime Database listener and a built-in simulator for offline testing
- Displays real-time BPM with heart rate classification (Bradycardia / Normal / Elevated / Tachycardia)
- Renders a live scrolling **Chart.js line chart** of BPM history
- Maintains a **session history table** with timestamps and classification labels
- Supports **PDF export** (via jsPDF) and **CSV export** of session data
- Includes a **BMI calculator** panel
- Shows Firebase connection status and supports reconnection
- Fully styled dark-mode UI with CSS custom properties and a cyan/green accent palette

---

## ⚙️ Setup & Run Instructions

### Prerequisites
- Arduino IDE with ESP8266 board support installed
- A Firebase project with Realtime Database enabled
- A Wi-Fi network
- MAX30102 sensor wired to the ESP8266 via I²C (SDA → GPIO4, SCL → GPIO5)

### 1. Clone the Repository
```bash
git clone https://github.com/<your-username>/IoT-Based-Heart-Rate-Monitoring-System.git
cd IoT-Based-Heart-Rate-Monitoring-System
```

### 2. Install Arduino Libraries
In the Arduino IDE, install the following via **Sketch → Include Library → Manage Libraries**:
- `FirebaseESP8266` by Mobizt
- `SparkFun MAX3010x Pulse and Proximity Sensor Library`

### 3. Configure Credentials
Open `heart_rate_monitor_aurdino.ino` and fill in your credentials:

```cpp
#define WIFI_SSID      "your_wifi_ssid"
#define WIFI_PASSWORD  "your_wifi_password"
#define FIREBASE_HOST  "your-project.firebaseio.com"
#define FIREBASE_AUTH  "your_firebase_database_secret"
```

> ⚠️ Never commit real credentials. Consider using a separate `secrets.h` file and adding it to `.gitignore`.

### 4. Upload to ESP8266
- Select **NodeMCU 1.0 (ESP-12E Module)** as the board
- Upload `heart_rate_monitor_aurdino.ino`
- Open Serial Monitor at **115200 baud** to observe live readings

### 5. Launch the Web Dashboard
Open `index.html` in any modern browser. Update the Firebase config block inside the `<script>` section with your project credentials, then:
- Select **Firebase** mode to receive live data
- Or select **Simulator** mode to test the dashboard offline

---

## 🚀 How It Works

1. Place your finger on the MAX30102 sensor — the firmware detects contact via the IR threshold
2. A 30-second **measurement round** begins automatically; the RGB LED turns cyan while calibrating, then green once a valid BPM is locked
3. After 30 seconds, the round average is printed to Serial and pushed to Firebase; a new round starts immediately
4. Every two rounds, a **combined average** is calculated and pushed to `/sessions/combined_avg`
5. The web dashboard listens to Firebase in real time and updates the BPM display, chart, and history table live
6. Export the session log as a PDF or CSV from the dashboard at any time

---

## 🎨 RGB LED Status Indicators

| Color | Meaning |
|---|---|
| White | No finger detected |
| Yellow | Finger partially detected (calibrating) |
| Cyan | Finger placed — awaiting first valid BPM |
| Green | Normal heart rate (60–100 BPM) |
| Blue | Low heart rate (< 60 BPM) |
| Red | High heart rate (> 100 BPM) |
| Purple (blinking) | MAX30102 sensor not found — check wiring |

---

## 👥 Team Members

| Name | GitHub |
|---|---|
| Noor Nabeeha | [@noornabeeha](https://github.com/noornabeeha) |
| Riteesha Banavannavar | [@rit2006](https://github.com/rit2006) |
| Sneha Shukla | [yetToBeFilled] |
| Kavita Iyer | [@KaviiiitaIyer](https://github.com/KaviiiitaIyer) |
| Padamshri Deora | [@padamshri](https://github.com/padamshri) |

---

## 🤖 AI Tools Declaration

The following AI tools were used during the development of this project:

| Tool | Usage |
|---|---|
| Claude (Anthropic) | Generating the BPM peak-and-valley detection algorithm structure; drafting the Firebase push logic; suggesting Chart.js configuration for the live BPM chart; writing and formatting this README |

This declaration is made in the interest of academic integrity and transparency. All AI-assisted content was reviewed, verified, and adapted by the team.

---

*Last updated: May 2026*
