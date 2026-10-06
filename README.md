# ESP32-CAM Smart Doorbell with Telegram Integration

## Description
This smart doorbell project is based on the ESP32-CAM module. When someone rings your doorbell, it automatically captures a photo — with or without flash, depending on user preference — and sends it to the homeowner via Telegram. 

It also supports remote control through Telegram commands so you’re always informed about what’s happening at your front door, no matter where you are!

![Doorbell Preview](assets/doorbell.jpg)

## Features
- **Auto-Capture:** Takes a picture automatically when the doorbell button is pressed.
- **Telegram Notifications:** Instantly sends the captured image to your phone.
- **Remote Commands (Telegram Bot):**
  - `/photo` : Takes a new snapshot on demand.
  - `/flash` : Toggles the flash on or off.
  - `/ringtone` : Switches between two different buzzer ringtones.
  - `/start` : Displays the available command menu.

## Required Components
- ESP32-CAM board
- FTDI programmer (for uploading the code)
- Push Button
- Buzzer
- Breadboard & Jumper Wires
- Power Supply (5V/1A recommended)

## Circuit Diagram
![Circuit Diagram](assets/circuit.jpg)

## Setup Instructions

### 1. Create a New Bot in Telegram
1. Search for **BotFather** in Telegram.
2. Tap on START.
3. Type `/newbot` and press enter.
4. Give a unique name for the BOT.
5. Enter a unique username (must end with "bot", e.g., `my_doorbell_bot`).
6. Save the **Bot Token** provided. You will need it in the code.

### 2. Get Your Telegram User ID
1. Search for **IDBot** in Telegram.
2. Tap on START.
3. Type `/getid`.
4. Note your **User ID**. You will need it in the code so the bot only answers to you.

### 3. IDE Configuration
1. Open Arduino IDE.
2. Go to `File > Preferences > Additional Board URLs` and paste:
   - `https://dl.espressif.com/dl/package_esp32_index.json`
   - `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
3. Go to `Tools > Board > Board Manager` and install the **ESP32** package by Espressif.
4. Install the following libraries via the Library Manager:
   - **UniversalTelegramBot** by Brian Lough
   - **ArduinoJson** by Benoit Blanchon

### 4. Uploading the Code
Before uploading, open the `.ino` file and replace the placeholders with your actual credentials:
```cpp
const char* ssid = "WiFi Name"; 
const char* password = "WiFi Password";
String chatId = "YOUR_USER_ID"; 
String BOTtoken = "YOUR_BOT_TOKEN";
