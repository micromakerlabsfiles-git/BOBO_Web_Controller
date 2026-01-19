BOBO 2.1: The Complete User Guide
This document is a comprehensive A-to-Z guide for setting up, using, and mastering your BOBO 2.1 RTOS. It covers hardware requirements, software installation, API generation, and how to control your robot using the provided web interface.
________________________________________
1. What is BOBO 2.1?
BOBO 2.1 is an advanced personality engine for ESP32-based robots. It turns a static display into a living, breathing character.
Key Features:
•	Ultra Pro Physics Engine: Simulates realistic eye movements, blinking, and "breathing" pupil dilation.
•	9 Distinct Moods: From Happy and Excited to Suspicious and Angry.
•	Smart Weather Station: Displays real-time temperature, humidity, and a 3-Day Forecast.
•	World Clock: Tracks two separate time zones simultaneously.
•	Web Control Center: A dedicated HTML dashboard to control every aspect of the robot via USB.
•	Boot Sequences: Custom animations (Gears, Rocket, Typing Text) on startup.
________________________________________
2. Hardware Requirements
To run this OS, you need the following components:
•	Microcontroller: ESP32 Development Board (e.g., ESP32 DevKit V1).
•	Display: 0.96" I2C OLED Display (SSD1306 driver).
•	Touch Sensor: TTP223 Capacitive Touch Sensor (connected to Pin 1).
•	USB Cable: For data transfer and power.
Wiring Connections:
•	OLED SDA → GPIO 21
•	OLED SCL → GPIO 20
•	Touch Sensor → GPIO 1
________________________________________
 
3. Software Setup (Installation Guide)
Step A: Install Arduino IDE
1.	Download and install the Arduino IDE.
2.	Open Arduino IDE and go to File > Preferences.
3.	In "Additional Boards Manager URLs", paste:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
4.	Go to Tools > Board > Boards Manager, search for ESP32, and install "esp32 by Espressif Systems".
Step B: Install Required Libraries
Go to Sketch > Include Library > Manage Libraries, search for and install these specific libraries:
1.	Adafruit GFX Library
2.	Adafruit SSD1306
3.	Arduino_JSON (by Arduino)
Step C: Prepare the Files
1.	Create a folder on your computer named BOBO_96_Web_Control.
2.	Move your main code file (BOBO_96_Web_Control.ino) into this folder.
3.	Create a new file inside the folder named boot_sequence.h.
4.	Paste the boot animation code (the one we generated previously) into boot_sequence.h.
5.	Important: Ensure you have pasted the long hex arrays for the Gears, Power Button, and Rocket animations into boot_sequence.h where indicated.
Step D: Upload
1.	Connect your ESP32 to your PC.
2.	Select your board in Tools > Board > DOIT ESP32 DEVKIT V1.
3.	Select the correct Port in Tools > Port.
4.	Click Upload (Arrow icon).
________________________________________
 
4. API Generation (Getting Weather Data)
To make the weather features work, you need a free "API Key" from OpenWeatherMap.
1.	Go to openweathermap.org and create a free account.
2.	Once logged in, click your username in the top right corner and select "My API keys".
3.	You will see a "Default" key, or you can create a new one named "BOBO".
4.	Copy this Key. It will look like a long string of random letters and numbers (e.g., a1b2c3d4e5...).
5.	Note: It may take 10-60 minutes for a new key to become active.
________________________________________
5. Using the Web Control Center
The BOBO_Web_Controller.html file is your control panel. It uses Web Serial technology, which allows a website to talk directly to your USB devices.
How to Connect:
1.	Open the HTML file in Google Chrome, Edge, or Opera (Firefox does not support Web Serial).
2.	Connect your ESP32 to the computer via USB.
3.	Click the "🔌 Connect" button on the webpage.
4.	Select your ESP32 (often labeled "USB UART" or "CP210x") from the popup list and click "Connect".
5.	The status dot will turn Green.
Features & Controls:
•	Network & API:
o	WiFi: Enter your SSID and Password, then click "Set SSID" and "Set Pass".
o	Weather: Enter your City (e.g., "Thrissur") and paste the API Key you generated in Step 4.
o	Save: Click "💾 Save WiFi & Time" to store these settings permanently on the robot.
•	World Clock: Select your two desired time zones (e.g., "New York" and "Tokyo") and click "Set Zone".
•	Eye Physics:
o	Use the sliders to change the eye size, roundness, and position.
o	Click "💾 Save Eye Shape" to make the robot remember your custom look even after rebooting.
•	Live Control:
o	Expression: Instantly change the mood (Happy, Suspicious, etc.).
o	Page: Force the screen to switch between Eyes, Clock, or Weather.
________________________________________
6. Update the boot animations
Quick guide on how to update the boot animations and text in the future. All your changes will happen in boot_sequence.h.
1. Changing the Text
To change what BOBO says on startup (e.g., "Hey Bro...", "Welcome"), scroll to the run BootSequence function at the bottom of the file.
•	Find this:
typeText(display, "Hey Bro...", 0, 15);
•	Edit it: Just change the text inside the quotes.
typeText(display, "Hello Master", 0, 15);
(The numbers 0, 15 are the X and Y coordinates. X=0 is left, Y=0 is top).
2. Changing the Animations
If you want to replace the "Gears" or "Rocket" with something else (like a waving hand or a heart), follow these steps:
Step A: Generate the Code
1.	Go to the Wokwi OLED Animator.
2.	Upload your collection of images or GIFs.
3.	Set the size to 128x64 (or 48x48 if you want it smaller like your current ones).
4.	Copy the generated const byte PROGMEM frames... code.
Step B: Paste into boot_sequence.h
1.	Open boot_sequence.h.
2.	Find the array you want to replace, for example, gears_frames.
3.	Delete everything inside the curly braces { ... } and paste your new numbers there.
Step C: Update Frame Count (Important) If your new animation has more or fewer frames than the old one, the code handles it automatically IF you kept the sizeof line directly below the array:
const int GEARS_COUNT = sizeof(gears_frames) / sizeof(gears_frames[0]);
As long as you don't delete this line, BOBO will automatically know how long the new animation is.
3. Changing Animation Speed
To make an animation play faster or slower, go to the playAnim function call inside runBootSequence (at the bottom of the file).
•	Find this:
// The '2' is the number of loops. 
playAnim(display, gears_frames, GEARS_COUNT, 2); 
•	To change speed: You would need to edit the playAnim helper function at the top of the file. Change delay(42); to a smaller number (faster) or larger number (slower).

