# ELEE 2045 Assignment 3: Distributed MQTT Security System

**Name:** [Your Name Here]  

<img src="images/headshot.jpg" alt="My Headshot" width="200"/>

---

## 🎥 Video Demonstration
[Link to Assignment 3 Video](https://youtube.com/...)

---

## 🏗️ System Architecture & MQTT Topics
> [Explain the overall architecture of your system. List the MQTT topics you used to communicate between the Python monitor and the M5StickCPlus. Briefly explain what type of data/messages are sent over each topic.]

**Topics Used:**
* `studentname/security/state` - [Example: Used to publish whether the system is Armed Home, Armed Away, or Off.]
* `studentname/security/command` - [Example: Used to subscribe to mode changes from the remote monitor.]
* `studentname/security/telemetry` - [Example: Used to publish sensor data and connection heartbeats.]

**Connection Monitoring:**
> [Explain how your Python program monitors the connection status of the M5StickCPlus (e.g., a heartbeat timer, Last Will and Testament, etc.) and what happens when it disconnects.]

---

## 🛡️ Hardware & Sensing Mechanism
> [Describe the physical property you are securing. What sensor(s) on the M5StickCPlus (IMU, microphone, external Grove sensor, etc.) did you use to detect a breach, and what is the logic behind triggering the alarm?]

---

## 🖥️ User Interface Guide

### M5StickCPlus (Local Control)
> [Provide a brief manual on how to interact with the M5StickCPlus. Which buttons arm/disarm the system? How do you change modes? How does the screen/device visually indicate its current mode or an alarm state?]

### Python Monitor (Remote Dashboard)
> [Explain how to use your Python application. How do you arm/disarm remotely? How is the telemetry data and connection status displayed to the user?]

---

## 🛠️ How to Build & Run
> [List any Python packages (like paho-mqtt) that need to be installed. Provide the exact command line instruction to run your Python monitor program.]

**Installation:**
`pip install paho-mqtt`

**Execution:**
`python monitor.py`

---

## ✨ Creative Extension
*Description of the additional feature implemented beyond the base requirements.*

**1. [Name of Extension]**
> [Describe what this feature does, how it works, and why you chose to implement it. For example: "I implemented a PC webcam integration that snaps a photo of the user and saves it to a local folder whenever the Away Mode grace period expires without being disarmed."]

---

## 📂 Repository Structure

* `/SecuritySensor` - Source code for the M5StickCPlus (Arduino/C++).
* `/SecurityMonitor` - Source code for the computer dashboard (Python).  
* `/images` - Folder containing assets for this README (like your headshot).