# ELEE 2045 Assignment 4: Bluetooth-Controlled Pygame Simulation

**Name:** Nikhil Mahadevan

<img src="images/headshot.jpg" alt="My Headshot" width="200"/>

## 🎥 Video Demonstration

Link to Assignment 4 Video: https://youtube.com/shorts/iRcOpoL3GqM?si=t_SdyiUuJsnKCGe1

> Note: Your video must be under 3 minutes, include a voiceover, and show the Pygame window and physical M5Stick side-by-side.

---

## 🚀 Project Description

This project implements a bidirectional BLE system connecting an M5Stick to a Pygame simulation. The goal was to create a hardware-integrated game where the physical device acts as both a high-fidelity sensor and a real-time feedback interface.

- **Objective:** Navigate a player through a virtual space using the M5Stick as a tilt-controller.  
- **Continuous Input:** Utilized the 3-axis accelerometer to map physical gravity vectors to player velocity in Pygame.  
- **Discrete Input:** Integrated Button A as a state-toggle, allowing the user to switch between "Blue" and "Green" modes, reflected on both the device screen and the simulation.  
- **Bidirectional Feedback:** When the Pygame engine detects a virtual collision, it writes a signal back to the M5Stick. The device reacts by:
  - Flashing the screen RED
  - Playing a 4kHz tone
  - Displaying the current score  

---

## 🛠️ How to Build & Run

Make sure you have the M5StickC Plus2 flashed with the Arduino C++ code and that your computer has Bluetooth enabled.

### Installation

```bash
pip install bleak pygame
```

### Execution

```bash
python part2.py
```

---

## ✨ Creative Extension: Collision Tracking System

- **Description**: Added a simple collision tracking system between the game and hardware UI.
- **How it Works**: When Python detects a collision, it sends the updated collision count to the M5Stick over BLE, which then shows the number of collisions and plays a short alert sound.