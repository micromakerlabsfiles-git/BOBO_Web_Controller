# 🤖 BOBO 2.2: Advanced Personality Engine
**Now with Daily Alarms, Smart Reminders, Inactivity Auto-Scroll, and Compact Web Serial Controls.**

BOBO 2.2 is an advanced personality engine for ESP32-based robots (specifically the C3 SuperMini). It transforms a simple OLED display into a living character using a physics-driven animation system, wellness reminders, and a full Web-based control dashboard.

This document covers hardware assembly, software installation, and full control via the dedicated Web Interface.

---

## ✨ What's New in v2.2?
- **⏰ Daily Alarm & Timer:** Set a daily alarm or a quick countdown timer directly from the Web UI or device. The countdown timer starts automatically upon double-tapping the clock.
- **💤 Low Power Mode:** Double-tap on the Eyes screen to put BOBO to sleep. It dims the display and shows a "sleeping" expression to save power.
- **💧 Wellness Reminders:** Optional hourly chime and hydration alerts to keep you drinking water.
- **💓 Physics-Driven Expressions:** 8 distinct moods (Normal, Happy, Surprised, Sleepy, Angry, Sad, Excited, Love) with realistic physics and corrected eyelid masking (slanted inner brows for Angry, drooping outer brows for Sad). The suspicious emotion has been removed.
- **🔄 Inactivity Auto-Scroll:** Automatically returns to the Eyes screen (page 0) after 20 seconds of inactivity on interactive screens (World Clock, Alarm View, Set Alarm, Settings menu, Music menu, or active Countdown) to save screen life and prevent NVS wear. Edits on Set Alarm are discarded if auto-scrolled, but saved if exited manually.
- **🎨 Web Controller Themes & Customization:** Switch between default Amber, Deep Blue, and a new High-Contrast Black & White theme. Customize which animations/texts run on boot using compact layout options.
- **🔌 Hardware Wiring Pinout Table:** Added a wiring reference table directly in the Flash tab of the web controller for physical flasher users.

---

## 📖 Table of Contents
- [Features](#-features)
- [Hardware Requirements](#-hardware-requirements)
- [Wiring Guide](#-wiring-guide)
- [Quick Start Guide](#-quick-start-guide)
- [Device Navigation & Gestures](#-device-navigation--gestures)
- [Software Setup](#-software-setup-installation-guide)
- [Web Control Center](#-using-the-web-control-center)
- [Customizing Boot Animations](#-update-the-boot-animations)

---

## 🌟 Features

* **Ultra Pro Physics Engine:** Simulates realistic eye movements, blinking, and "breathing" pupil dilation.
* **8 Distinct Moods:** Responsive expressions with custom slanting eyelids and sounds.
* **Positive Quotes Screen:** Shows 100 different positive quotes (min 6 words) rotating on each carousel screen scroll.
* **World Clock:** Tracks two separate time zones simultaneously on a split-screen view.
* **Web Control Center:** A dedicated HTML dashboard to control every aspect of the robot via USB (Web Serial) using Amber, Deep Blue, or Contrast themes.
* **Boot Sequences:** Toggles to customize startup sequences (Gears, Power, Rocket, Typing Text) directly from the browser.

---

## 🛠 Hardware Requirements

To build BOBO 2.2, you will need the following components:

| Component | Description |
| :--- | :--- |
| **Microcontroller** | ESP32 C3 SuperMini Development Board |
| **Display** | 0.96" or 1.3" I2C OLED Display (SSD1306 / SH110X driver) |
| **Touch Sensor** | TTP223 Capacitive Touch Sensor |
| **LED** | 1x WS2812B NeoPixel RGB LED (for visual alerts/effects) |
| **Buzzer** | A passive buzzer for sound notifications and tunes |
| **Connectivity** | Micro-USB Cable (Data + Power) |

---

## 🔌 Wiring Guide

Connect your components to the ESP32 C3 SuperMini using the following pin mapping:

| Component | Pin Name | ESP32 GPIO |
| :--- | :--- | :--- |
| **OLED Display** | SDA | `GPIO 20` |
| **OLED Display** | SCL | `GPIO 21` |
| **OLED Display** | VCC | `3.3V` or `5V` |
| **OLED Display** | GND | `GND` |
| **WS2812B LED** | DIN | `GPIO 6` |
| **WS2812B LED** | VCC | `5V` |
| **WS2812B LED** | GND | `GND` |
| **Touch Sensor** | I/O | `GPIO 1` |
| **Touch Sensor** | VCC | `3.3V` or `5V` |
| **Touch Sensor** | GND | `GND` |
| **Buzzer** | I/O | `GPIO 2` |

---

## 🚀 Quick Start Guide

1. **Connect & Power:** Connect BOBO to your computer using a Micro-USB cable.
2. **Flash the Firmware:** Open the Web Control Center in Chrome, Edge, or Opera. Go to the **Flash** tab, select your display variant (SSD1306 or SH110X), and click **Install / Update** to flash the device.
3. **Connect to Web Serial:** Go to the **Status** tab, click **Connect**, select the ESP32 UART port, and connect.
4. **Time Synchronization:** Connecting over Web Serial automatically syncs your local system time to the device.
5. **Configure Settings:** Customize boot sequences, daily alarms, home timezone offset, and audio cues in the **Settings** and **Display** tabs. The dashboard automatically saves settings directly to the device's NVS memory.

---

## 📱 Device Navigation & Gestures

BOBO has a single capacitive touch button (`GPIO 1`) for complete physical control.

### 🌟 Main Navigation (1 Tap)
- **Single Tap (1 Tap):** Cycle through the main screens:
  - `Eyes (Moods)` ➔ `Clock` ➔ `Date` ➔ `World Clock` ➔ `Alarm View` ➔ `Quotes` ➔ back to `Eyes`.

### 💤 Low Power, Timers, and Screen Transitions (2 Taps)
- **Double Tap (2 Taps):**
  - On the **Eyes** screen: Toggle **Low Power / Sleep Mode** (dims display contrast and sets sleeping eyes).
  - On the **Clock** screen (Digital or Analog): Automatically starts the **Countdown Timer**.
  - On the **World Clock** screen: Returns to the digital/analog Clock screen.
  - On the **Alarm View** screen: Enters **Alarm Setup Mode**.
  - On the **Quotes** screen: Returns directly to the **Eyes** screen.

### ⚙️ Open Settings Menu (3 Taps)
- **Triple Tap (3 Taps):** Open the **Settings Menu (Page 7)** instantly from any main screen.

### 🔄 Context Action (Long Press)
- On the **Eyes** screen: Cycles through standard moods (Normal ➔ Happy ➔ Surprised ➔ Sleepy ➔ Angry ➔ Sad ➔ Excited ➔ Love) and plays accompanying mood sound.
- On the **Clock** screen: Toggles between **Digital** and **Analog** clock faces.
- On the **Date** screen: Switches to the **Quotes** screen with a fresh random quote.
- On the **World Clock** screen: Switches to **Alarm View**.
- On the **Alarm View** screen: Enters **Alarm Setup Mode**.

---

### 🛠 Settings & Sub-menus Navigation

#### 1. Settings Menu (Page 7)
- **Single Tap:** Scroll down next settings item: *Negative Display ➔ Sound ➔ LED ➔ Music ➔ Exit*.
- **Double Tap:** Scroll up previous settings item.
- **Long Press:** Activate or toggle the selected setting (e.g. toggle Sound, enter LED sub-menu, or open Music menu).

#### 2. LED Sub-menu
- **Single Tap:** Scroll down next sub-menu item: *LED Status ➔ Brightness ➔ Effect ➔ Exit*.
- **Double Tap:** Scroll up previous sub-menu item.
- **Long Press:** Toggle or increment the selected option. If **Exit** is selected, return to the main Settings Menu.

#### 3. Music Menu (Page 8)
- **Single Tap:** Scroll down next melody (1 of 20 melodies or Exit).
- **Double Tap:** Scroll up previous melody.
- **Long Press:** Play the highlighted song. If **Exit** is selected, return to the main Settings Menu.

#### 4. Alarm Setup Mode (Page 6)
- **Single Tap:** Select next field: *Hour ➔ Minute ➔ Alarm On/Off ➔ Exit*.
- **Double Tap:** Select previous field.
- **Long Press:**
  - On Hour/Min/State: Increment the active field.
  - On **Exit**: Saves the changes to NVS Preferences and returns to the Alarm View page.

#### 5. Active Countdown Timer Screen
- **Single Tap:** Play / Pause the countdown timer.
- **Long Press:** Reset the countdown timer back to the default duration.
- **Double Tap:** Exit the countdown screen and return to the Eyes screen.

---

## 💻 Software Setup (Installation Guide)

### Step A: Install Arduino IDE or PlatformIO
1. **PlatformIO (Recommended):** Open the project folder in VS Code with PlatformIO. PlatformIO will automatically download the correct ESP32 core and library dependencies.
2. **Arduino IDE:**
   - Go to **File > Preferences** and add the ESP32 board URL to "Additional Boards Manager URLs":
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - In **Tools > Board > Boards Manager**, install `esp32` by Espressif Systems.

### Step B: Install Required Libraries
If using Arduino IDE, install these libraries via **Library Manager**:
1. `Adafruit GFX Library`
2. `Adafruit SSD1306`
3. `Adafruit SH110X`
4. `Adafruit NeoPixel`
5. `Arduino_JSON`

### Step C: Upload
1. Select your target environment:
   - For 0.96" standard screen: Compile target `ssd1306`
   - For 1.3" alternative screen: Compile target `sh110x`
2. Connect your device and upload.

---

## 🎛 Using the Web Control Center

The `index.html` file provides a local control dashboard communicating with BOBO via **Web Serial**.

### How to Connect
1. Open the [Web Control Center](https://micromakerlabsfiles-git.github.io/BOBO_Web_Controller/) in **Chrome, Edge, or Opera**.
2. Connect the ESP32 via USB and click **Connect**.
3. Choose the corresponding UART port. Time will automatically synchronize on connection.

### Dashboard Tabs
- **Status:** Connect/Disconnect, sync settings, serial logs, and reboot commands.
- **Eyes:** Adjust physical pupil dimensions, offset positions, scaling widths, and corner roundness.
- **Display:** Invert screen, change brightness, trigger moods (Normal, Happy, Surprised, Sleepy, Angry, Sad, Excited, Love), force page switches, set Home Timezone, and set World Clock zones.
- **Alarms:** Adjust the daily alarm time and toggle status, and configure the countdown timer duration.
- **Sound:** Toggle touch sounds, chimes, and hourly water reminders. Choose and play any of the 20 tunes.
- **Settings:** Compact grid adjustments for the WS2812B LED (on/off, brightness 1-5, effects solid/blink/pulse, custom RGB hex colors), boot text string updates, and boot phase customization toggles.
- **Flash:** Browser-based flashing utility for SSD1306/SH110X bin files and physical flasher pinout table.
