# BOBO 2.1: The Complete User Guide

**BOBO 2.1** is an advanced personality engine for ESP32-based robots. It transforms a static display into a living, breathing character with simulated physics, distinct moods, and real-time environmental awareness.

This document covers hardware assembly, software installation, API generation, and full control via the dedicated Web Interface.

---

## 📖 Table of Contents
- [Features](#-features)
- [Hardware Requirements](#-hardware-requirements)
- [Wiring Guide](#-wiring-guide)
- [Software Setup](#-software-setup-installation-guide)
- [API Generation (Weather)](#-api-generation-getting-weather-data)
- [Web Control Center](#-using-the-web-control-center)
- [Customizing Boot Animations](#-update-the-boot-animations)

---

## 🌟 Features

* **Ultra Pro Physics Engine:** Simulates realistic eye movements, blinking, and "breathing" pupil dilation.
* **9 Distinct Moods:** From Happy and Excited to Suspicious and Angry.
* **Smart Weather Station:** Displays real-time temperature, humidity, and a 3-Day Forecast.
* **World Clock:** Tracks two separate time zones simultaneously.
* **Web Control Center:** A dedicated HTML dashboard to control every aspect of the robot via USB (Web Serial).
* **Boot Sequences:** Custom animations (Gears, Rocket, Typing Text) on startup.

---

## 🛠 Hardware Requirements

To run BOBO 2.1, you will need the following components:

| Component | Description |
| :--- | :--- |
| **Microcontroller** | ESP32 C3 SuperMini Development Board - [Know more about board](https://randomnerdtutorials.com/getting-started-esp32-c3-super-mini/) |
| **Display** | 0.96" I2C OLED Display (SSD1306 driver) |
| **Touch Sensor** | TTP223 Capacitive Touch Sensor |
| **Connectivity** | Micro-USB Cable (Data + Power) |

---

## 🔌 Wiring Guide

Connect your components to the ESP32 using the following pin mapping:

| Component | Pin Name | ESP32 GPIO |
| :--- | :--- | :--- |
| **OLED Display** | SDA | `GPIO 21` |
| **OLED Display** | SCL | `GPIO 20` |
| **OLED Display** | VCC | `3.3V` or `5V` |
| **OLED Display** | GND | `GND` |
| **Touch Sensor** | I/O | `GPIO 1` |
| **Touch Sensor** | VCC | `3.3V` or `5V` |
| **Touch Sensor** | GND | `GND` |

**Circuit Diagram**

![Slide1](https://github.com/user-attachments/assets/33e228d0-2e5d-4404-971a-137fec8e2aaf)


---

## 💻 Software Setup (Installation Guide)

### Step A: Install Arduino IDE
1.  Download and install the [Arduino IDE](https://www.arduino.cc/en/software).
2.  Open the IDE and navigate to **File > Preferences**.
3.  In the "Additional Boards Manager URLs" field, paste the following link:
    ```text
    [https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json](https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json)
    ```
4.  Go to **Tools > Board > Boards Manager**, search for **ESP32**, and install "esp32 by Espressif Systems".

### Step B: Install Required Libraries
Navigate to **Sketch > Include Library > Manage Libraries**, then search for and install these specific libraries:

1.  `Adafruit GFX Library`
2.  `Adafruit SSD1306`
3.  `Arduino_JSON` (by Arduino)

### Step C: Prepare the Files
1.  Create a folder on your computer named `BOBO_96_Web_Control`.
2.  Place your main code file (`BOBO_96_Web_Control.ino`) inside this folder.
3.  Create a new file inside the folder named `boot_sequence.h`.
4.  Paste your generated boot animation code (hex arrays for Gears, Rocket, etc.) into `boot_sequence.h`.

### Step D: Upload
1.  Connect your ESP32 to your PC.
2.  Select your board: **Tools > Board > MakerGO ESP32 C3 SuperMini**.
3.  Select the correct Port: **Tools > Port**.
4.  Click **Upload** (Arrow icon).

---

## ☁️ API Generation (Getting Weather Data)

To enable weather features, you need a free API Key from OpenWeatherMap.

1.  Go to [openweathermap.org](https://openweathermap.org/) and create a free account.
2.  Click your username (top right) and select **My API keys**.
3.  Create a new key named "BOBO".
4.  Copy the key (e.g., `a1b2c3d4e5...`).

> **Note:** It may take 10–60 minutes for a newly generated key to become active on their servers.

---

## 🎛 Using the Web Control Center

The `BOBO_Web_Controller.html` file acts as your control panel. It utilizes **Web Serial API**, allowing the browser to communicate directly with your USB devices.

### How to Connect
1.  Open [BOBO_Web_Controller](https://micromakerlabsfiles-git.github.io/BOBO_Web_Controller/) in **Chrome, Edge, or Opera** (Firefox is not supported).
2.  Connect the ESP32 via USB.
3.  Click the **🔌 Connect** button on the page.
4.  Select your device (usually "USB UART" or "CP210x") and click **Connect**. The status dot will turn Green.

### Features & Controls
* **Network & API:**
    * Enter WiFi SSID and Password → Click **Set SSID** / **Set Pass**.
    * Enter City Name and API Key → Click **Save WiFi & Time**.
* **World Clock:** Select two time zones (e.g., "New York", "Tokyo") and click **Set Zone**.
* **Eye Physics:** Adjust sliders for eye size, roundness, and position. Click **💾 Save Eye Shape** to persist settings after reboot.
* **Live Control:** Instantly trigger moods (Happy, Suspicious) or force page switches (Eyes/Clock/Weather).

---

## 🎨 Update the Boot Animations

All startup animations are handled in `boot_sequence.h`.

### 1. Changing the Text
To change the startup greeting, find the `runBootSequence` function at the bottom of the file:

```cpp
// Change the text inside quotes
typeText(display, "Hello Master", 0, 15);
// (0, 15 represents X and Y coordinates)
```

### 2. Changing the Animations
To replace animations (like the "Gears" or "Rocket"):

1. Go to the Wokwi OLED Animator.
2. Upload your images/GIFs (Resize to 128x64 or 48x48).
3. Copy the generated const byte PROGMEM frames... code.
4. In boot_sequence.h, replace the content of the existing array (e.g., gears_frames) with your new code.

Important: Do not delete the size calculation line below the array: const int GEARS_COUNT = sizeof(gears_frames) / sizeof(gears_frames[0]);

### 3. Changing Animation Speed
To adjust speed, modify the playAnim call in runBootSequence:
```cpp
// The '2' is the number of loops.
playAnim(display, gears_frames, GEARS_COUNT, 2);
```
To fine-tune the delay, edit the playAnim helper function at the top of the file and change delay(42); to a smaller number (faster) or larger number (slower).


