# CO₂-Sensor (ESP32 → MQTT → Desktop-Overlay)

A small, deliberately simple CO₂ monitoring setup for home use:  
An ESP32 reads a CO₂ sensor and transmits values via WiFi using **MQTT** to your network.  
On your PC, a **Python/Tkinter overlay** receives the data, evaluates it (color-coded indicator), and displays it as a small always-on-top widget including power consumption history.

> **Motivation:** At a workshop, there was a large CO₂ display – the idea immediately grabbed me. This version has been running reliably in my daily life for over a year.

![Example: CO₂ Overlay & Sensor](docs/images/setup.png)

---

## Features

- CO₂ value (eCO₂) displayed as **ppm** in the overlay
- **Color status** (gray/orange/red) based on configurable thresholds
- Display of additional data: **Power (W)** from an energy meter MQTT topic
- **History graph** (currently: 8 hours at 10-minute intervals → 48 data points)
- Overlay characteristics:
  - Always on top (`topmost`)
  - Frameless window (`overrideredirect`)
  - Transparent background (background color "grey" becomes transparent)

---

## Hardware Components

### Main Controller
- **ESP32** (affordable, WiFi built-in, sufficient processing power)

### CO₂ Sensors (Supported Examples)
The following sensors are based on NDIR technology and suitable for indoor air quality measurements:
- Sensirion **SCD30** or **SCD40** (I²C interface)
- Senseair **S8** (UART/Modbus interface)
- **MH-Z19C** (UART interface)
- **ENS160** (I²C interface - currently used in the included firmware)

### Additional Sensors (in current firmware)
- **AHT21** (temperature & humidity compensation for ENS160)
- **SSD1306 OLED display** (128x32 pixels, for local readings)

---

## Architecture / Data Flow

1. **ESP32** measures CO₂ every ~2 seconds (plus temperature/humidity if available).
2. **ESP32** publishes data via MQTT to your local network broker.
3. **PC script** `co2.py` subscribes to the MQTT topics, evaluates the data, and displays it in the overlay widget.

```
┌─────────────┐         ┌──────────────┐         ┌─────────────────┐
│   ESP32     │  MQTT   │ MQTT Broker  │  MQTT   │  PC Overlay     │
│  + Sensor   ├────────►│ (Mosquitto)  ├────────►│  (co2.py)       │
│  + Display  │ WiFi    │              │         │  Tkinter Widget │
└─────────────┘         └──────────────┘         └─────────────────┘
```

---

## MQTT Topics & Payload

### CO₂ Data
- **Topic:** `sensor/co2`
- **Payload (JSON):** Minimum expected format:
  ```json
  {
    "eco2": 624,
    "temp": 21.5,
    "humidity": 45,
    "aqi": 2,
    "tvoc": 120
  }
  ```
  - `eco2`: Equivalent CO₂ in ppm (required)
  - `temp`: Temperature in °C (optional)
  - `humidity`: Relative humidity in % (optional)
  - `aqi`: Air Quality Index 1-5 (optional)
  - `tvoc`: Total Volatile Organic Compounds in ppb (optional)

### Power/Energy Meter (Optional)
- **Topic:** `sensor/stromzaehler/SENSOR`
- **Payload (JSON):**
  ```json
  {
    "GS303": {
      "Power_cur": 50
    }
  }
  ```
  - `Power_cur`: Current power consumption in Watts

**Note:** If you use different topics or JSON keys, adjust them in the `on_message()` function in `co2.py`.

---

## Installation – PC Overlay

### Prerequisites
- **Windows** (recommended for included launcher; `co2.py` also works on Linux/macOS with minor adjustments)
- **Python 3.x**
- **MQTT Broker** accessible on your network (e.g., Mosquitto on NAS/Raspberry Pi/server)

### Python Dependencies
Install required packages:
```bash
pip install paho-mqtt
```

*Note: Tkinter is included with most Python installations. On Linux, you may need to install it separately (e.g., `apt install python3-tk`).*

### Starting the Overlay

#### Method 1: Direct Python Execution
1. Edit `co2.py` and update the broker IP address:
   ```python
   client.connect("192.168.178.151")  # Change to your broker's IP
   ```

2. Run the script:
   ```bash
   python co2.py
   ```

#### Method 2: Using a Launcher Script (Windows)
Create a batch file `co2.bat` to start the overlay minimized without a console window:
```batch
@echo off
start /min pythonw.exe "C:\path\to\co2.py"
```

**Important:** Adjust the path to match your system.

#### Other Operating Systems
- **Linux/macOS:** Create a shell script or desktop entry to launch `co2.py`
- Consider adding to startup/autostart for automatic launch on login

---

## ESP32 Firmware – Build & Upload

The ESP32 firmware is located in the **`CO2_ESP32/`** directory and uses **PlatformIO** for building and uploading.

### Prerequisites
- [PlatformIO Core](https://platformio.org/install) or [PlatformIO IDE](https://platformio.org/platformio-ide) (VS Code extension recommended)
- USB cable for ESP32 programming

### Quick Start with PlatformIO

1. **Navigate to the firmware directory:**
   ```bash
   cd CO2_ESP32
   ```

2. **Configure WiFi and MQTT settings:**
   Edit `src/main.cpp` and update the following lines:
   ```cpp
   #define WLAN_SSID     "YourWiFiSSID"
   #define WLAN_PASS     "YourWiFiPassword"
   #define MQTT_BROKER   "192.168.1.100"  // Your MQTT broker IP
   #define MQTT_PORT     1883
   #define MQTT_TOPIC    "sensor/co2"
   ```

3. **Build the firmware:**
   ```bash
   pio run
   ```

4. **Upload to ESP32:**
   ```bash
   pio run --target upload
   ```

5. **Monitor serial output (optional):**
   ```bash
   pio device monitor
   ```

### Supported Boards
The current configuration targets `wemos_d1_mini32`. To use a different ESP32 board:
- Edit `platformio.ini` and change the `board` parameter
- See [PlatformIO ESP32 boards](https://docs.platformio.org/en/latest/boards/index.html#espressif-32) for options

### Dependencies
The following libraries are automatically downloaded by PlatformIO (see `platformio.ini`):
- `Adafruit AHTX0` – Temperature & humidity sensor
- `ScioSense ENS16x` – ENS160 air quality sensor
- `Adafruit SSD1306` – OLED display driver
- `PubSubClient` – MQTT client

---

## Configuration & Customization

### CO₂ Threshold Levels (in co2.py)
Adjust the color-coded warning thresholds in `co2.py`:
```python
if eco2_value >= 1100:
    co2_color = 'red'      # High CO₂ - ventilation needed
elif eco2_value >= 800:
    co2_color = 'orange'   # Moderate CO₂ - monitor
else:
    co2_color = 'lightgrey'  # Good air quality
```

Recommended thresholds:
- **< 800 ppm:** Good air quality
- **800-1100 ppm:** Moderate; ventilation recommended
- **> 1100 ppm:** Poor air quality; immediate ventilation needed

### Graph History (in co2.py)
Currently configured for 48 data points:
```python
power_history = [0] * 48  # 8h at 10min intervals (48*10min=480min=8h)
```

**Note:** The current implementation appends a value on every MQTT power update, not at fixed 10-minute intervals. For true time-based sampling, implement:
- Timer-based sampling in the client, or
- Server-side aggregation (e.g., Node-RED/Telegraf/InfluxDB)

### BAT Launcher Configuration (Windows)
If using a `.bat` file for autostart:
- Update hardcoded paths to match your system
- Place in Windows Startup folder for automatic launch
- Use `pythonw.exe` instead of `python.exe` to hide the console window

### Code Adaptation Hints
- **Different sensors:** Modify `src/main.cpp` to read from your specific sensor (SCD30, S8, MH-Z19C)
- **Additional data:** Extend the JSON payload in both ESP32 firmware and `co2.py` parsing
- **Display customization:** Adjust Tkinter widget size, colors, and layout in `co2.py`

---

## Project Structure

```
CO2-Sensor/
├── co2.py                      # Python/Tkinter overlay application
├── CO2_ESP32/                  # ESP32 firmware (PlatformIO project)
│   ├── platformio.ini          # PlatformIO configuration
│   ├── src/
│   │   └── main.cpp            # Main firmware code (ENS160 + AHT21)
│   ├── include/                # Header files
│   ├── lib/                    # Local libraries
│   └── test/                   # Unit tests
├── docs/                       # Documentation and resources
│   └── images/
│       └── setup.png           # Example setup photo
└── README.md                   # This file
```

---

## Roadmap / Future Ideas

- [x] Add ESP32/C++ firmware to repository
- [ ] Improve auto-reconnect and heartbeat stability for MQTT
- [ ] Add data logging (CSV/InfluxDB/SQLite)
- [ ] Implement dynamic scaling in the power graph
- [ ] Create configuration file (`config.json`) for broker/topics/thresholds
- [ ] Support for multiple sensor types (SCD30, S8, MH-Z19C) with auto-detection
- [ ] Web-based configuration interface for ESP32
- [ ] Historical data visualization and trends
- [ ] Mobile app or web dashboard
- [ ] Alert notifications (push/email) for high CO₂ levels

---

## Troubleshooting

### Overlay doesn't receive data
- Check that MQTT broker is running and accessible
- Verify broker IP address in `co2.py` matches your broker
- Ensure ESP32 is connected to WiFi and publishing to correct topic
- Test MQTT connectivity: `mosquitto_sub -h BROKER_IP -t sensor/co2`

### ESP32 won't connect
- Verify WiFi credentials in `main.cpp`
- Check serial monitor output for error messages
- Ensure MQTT broker allows connections from ESP32's IP
- Check that I²C sensors are properly wired and powered

### Display shows "---" values
- MQTT connection may not be established
- Topic name mismatch between ESP32 and `co2.py`
- JSON payload format doesn't match expected structure

---

## License
No license specified yet. Consider: MIT, Apache-2.0, GPL-3.0, etc.
