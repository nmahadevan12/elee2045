# ELEE 2045 Assignment 3: MQTT Security System

**Name:** Nikhil Mahadevan

<img src="images/headshot.jpg" alt="My Headshot" width="200"/>

---

## Video Demonstration
[Assignment 3 Video](https://youtube.com/shorts/u76So-p4q6w?si=W4O7LCeYNzsaCHWi)

---

## System Architecture & MQTT Topics

The system follows a distributed architecture using an M5StickCPlus2 as the local edge sensor and a Flask application as the remote monitoring dashboard. Communication is handled via an MQTT broker using a packed binary payload format (`#pragma pack(1)` in C++ and `struct.unpack` in Python) to ensure efficient data transmission.

### Topics Used

- `elee2045sp26/nm25652/doorSensor`  
  Publishes real-time sensor state changes (OPEN/CLOSED).

- `elee2045sp26/nm25652/mode`  
  A bidirectional topic used to sync system states (OFF, HOME, AWAY) between the physical device and the web dashboard.

- `elee2045sp26/nm25652/heartbeat`  
  Sent by the M5StickCPlus2 every 10 minutes to provide periodic status updates and connection telemetry.

### Connection Monitoring

The Python monitor (`part2.py`) tracks the `last_ts` timestamp of the last received message. If the time elapsed since the last communication exceeds 30 minutes, the dashboard visually toggles from ONLINE to OFFLINE.

---

## Hardware & Sensing Mechanism

This system secures a door using a Reed Switch connected to GPIO 26 of the M5StickCPlus2.

### The Sensor
The reed switch is held closed by a magnet when the door is shut. When the door opens, the circuit opens, triggering a state change.

### Alarm Logic

- In AWAY mode, a breach triggers a 10-second disarm countdown displayed on the M5 screen.
- If the system is not turned to OFF within that window:
  - The M5 speaker emits a 4000 Hz alarm tone
  - The screen turns red
  - An "ALARM!" warning is displayed.

---

## User Interface Guide

### M5StickCPlus2 (Local Control — `part1.ino`)

- Button A: Cycles through security modes  
  `OFF → HOME → AWAY`

- Visual Feedback
  - Black Screen: Normal system status
  - Yellow Text: Active DISARM countdown during a breach in AWAY mode
  - Red Screen: Active alarm state with audible siren

---

### Python Monitor (Remote Dashboard — `part2.py`)

- Dashboard: Displays real-time door status (OPEN / CLOSED) and the current system mode.
- Remote Control: Interactive buttons (OFF, HOME, AWAY) allow the user to change the mode remotely via MQTT.
- Connectivity Indicator: Shows whether the hardware is currently communicating with the server.

---

## How to Build & Run

### Installation

Install the required Python packages:

```bash
pip install paho-mqtt flask requests
```

---

## Execution

### Hardware

Open `part1/part1.ino` in the Arduino IDE.

Ensure the following libraries are installed:

- M5StickCPlus2
- PubSubClient

Update the WiFi credentials and upload the program to your device.

### Software

Run the Python monitor script:

```bash
python part2.py
```

### Access

Open your browser and navigate to:

```
http://localhost:5000
```

---

## Creative Extension: Discord Webhook Notifications

### Feature: Instant Alert System

I implemented Discord webhooks to recieve real-time notifications.

The Python backend monitors the MQTT stream and automatically triggers a Discord notification whenever the security mode is changed or the Door is opened or closed.

This provides an additional layer of security by sending push notifications directly to the user's phone or laptop making sure alerts are received even when the user is not actively monitoring the dashboard.

---

## Repository Structure

```
/part1
 └── part1.ino          # M5StickCPlus2 Source Code

part2.py                # Flask web dashboard and MQTT monitoring logic

/images                 # Folder containing assets for this README

/requirements.txt       # File containing specific libraries needed
```