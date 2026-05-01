# ELEE 2045 Lab 2: Serial Communication & States

**Name:** Nikhil Mahadevan

<img src="images/elee_2045_picture.jpg" alt="My Headshot" width="200"/>

---

## 🚀 Getting Started

### Clone the Repository

```bash
git clone <repository-url>
cd assignment-2-nmahadevan12
```

### Project Overview

This assignment contains two projects:
- **Part 1:** Standalone Reaction Game (M5)
- **Part 2:** Digital Photo Frame (M5 + Python)

Select the appropriate section below based on which project you'd like to run.

---

## Part 1: Standalone Reaction Game

### 🎥 Video Demonstration

[**Link to Part 1 Video**](https://youtu.be/siFMDEkv4Ms)

### 🛠️ How to Build & Run

To run this program, you must set up the M5 IDE for the M5StickC Plus 2.

#### 1. Hardware Requirements
* **Device:** M5StickC Plus 2
* **Cable:** USB-C Data Cable

#### 2. Software Prerequisites
* [M5 Arduino IDE](https://www.arduino.cc/en/software) (Version 2.0 or later recommended)

#### 3. Installation Steps

**Step A: Install the Board Manager**
1. Open M5 Arduino IDE and go to **Settings/Preferences**.
2. In "Additional Boards Manager URLs", add:
   `https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json`
3. Go to **Tools > Board > Boards Manager**, search for `M5Stack`, and click **Install**.

**Step B: Install the Library**
1. Go to **Sketch > Include Library > Manage Libraries**.
2. Search for **M5StickCPlus2** (Ensure it is the "Plus2" version).
3. Click **Install**.

**Step C: Upload the Code**
1. Connect your M5StickC Plus 2 to the computer.
2. Select your board: **Tools > Board > M5Stack Arduino > M5StickCPlus2**.
3. Select your port and click the **Upload** button.

### ▶️ How to Run

1. After uploading, the M5StickC Plus 2 will automatically start the reaction game.
2. Press the button on the device when the screen changes color to test your reaction time.
3. Your reaction time and running average will be displayed on the screen.
4. Press the button again to restart and play again!

---

## Part 2: Digital Photo Frame

### 🎥 Video Demonstration

[**Link to Part 2 Video**](https://youtu.be/2k39mxTQrhk)

### 🛠️ How to Build & Run

To run the photo frame, you need to set up both the M5Stick (Receiver) and a Python script (Sender) on your computer.

#### 1. Flash the M5Stick
1. Open the Part 2 `.ino` file in M5 Arduino IDE.
2. Ensure the **M5StickCPlus2** library is installed.
3. Connect your device and click **Upload**.
   * *The screen will display "Waiting for photos..." once ready.*

#### 2. Python Environment Setup
You must install the required Python libraries to handle Serial communication and Image processing.
1. Open your terminal or command prompt.
2. Install dependencies:
   ```bash
   pip install pyserial pillow
   ```

#### 3. Run the Sender Script

1. Open your terminal and navigate to the project directory:
   ```bash
   cd assignment-2-nmahadevan12
   ```

2. Run the Python script with a folder containing images:
   ```bash
   python photoframe.py --folder /path/to/image/folder
   ```
   
   Example:
   ```bash
   python photoframe.py --folder ./test_pics
   ```

3. The script will:
   - Connect to your M5Stick via serial port
   - Read images from the specified folder
   - Send images to the device for display
   - Cycle through images automatically

---

## ✨ Creative Extensions

### Part 1: Reaction Game

#### 1. Running Average Statistics
**Description:**
Instead of showing only the most recent result, the device tracks the last 5 successful attempts and calculates a running average that is displayed as `Avg:` on the results screen.

#### 2. Visual Performance Bar
**Description:**
A graphical bar at the bottom of the results screen visualizes reaction speed with color coding:
- Green: Fast reaction (< 250ms)
- Yellow: Average reaction (250-400ms)
- Red: Slow reaction (> 400ms)

### Part 2: Photo Frame

#### 1. Two-Stage Serial Handshake
**Description:**
Uses "A" and "D" handshake for flow control to prevent the M5Stick's serial buffer from overflowing and ensure reliable image transmission.

#### 2. Image Processing Pipeline
**Description:**
Automatically resizes images to 240x135 pixels and compresses to JPEG (quality 85) for efficient transmission over serial connection.

---

## Project Structure

```
assignment-2-nmahadevan12/
├── part1/
│   └── part1.ino              # M5 Reaction Game Code
├── part2/
│   └── part2.ino              # M5 Photo Frame Receiver Code
├── photoframe.py              # Python Photo Frame Sender Script
├── test_pics/                 # Example Images for Testing
├── images/                    # Headshot Image
├── README.md                  # This File
└── requirements.txt           # Python/M5 Dependencies
```
