# CO₂-Sensor (ESP32 → MQTT → Desktop-Overlay)

Ein kleines, bewusst einfach gehaltenes CO₂-Mess-Setup für zuhause:  
Ein ESP32 liest einen CO₂-Sensor aus und sendet die Werte per WLAN via **MQTT** ins Netzwerk.  
Auf dem PC läuft ein **Python/Tkinter-Overlay**, das die Daten empfängt, bewertet (Farbampel) und als kleines Always-on-Top-Widget inkl. Power-Verlauf anzeigt.

> Motivation: Bei einem Workshop gab es eine große CO₂-Anzeige – die Idee hat mich sofort gepackt. Seit über einem Jahr läuft diese Version bei mir zuverlässig im Alltag.

![Beispiel: CO₂-Overlay & Sensor](docs/images/setup.png)

---

## Features

- CO₂-Wert (eCO₂) als **ppm** im Overlay
- **Farbstatus** (grau/orange/rot) abhängig von Schwellenwerten
- Anzeige zusätzlicher Daten: **Power (W)** von einem Stromzähler-Topic
- **Verlaufsdiagramm** (aktuell: 8 Stunden bei 10-Minuten-Takt → 48 Punkte)
- Overlay ist:
  - immer im Vordergrund (`topmost`)
  - ohne Fensterrahmen (`overrideredirect`)
  - mit Transparenzfarbe (Hintergrund „grey“ wird transparent)

---

## Komponenten (Hardware)

### Zentraleinheit
- **ESP32** (günstig, WLAN integriert, genug Rechenleistung)

### CO₂-Sensor (Beispiele)
Alle genannten Sensoren basieren auf NDIR-Technologie und sind für Raumluftmessungen geeignet:
- Sensirion **SCD30** oder **SCD40**
- Senseair **S8**
- **MH‑Z19C**

> Der C++/ESP32-Code ist in diesem Repo noch nicht enthalten und wird später ergänzt.

---

## Architektur / Datenfluss

1. ESP32 misst alle ~2 Sekunden CO₂ (und ggf. Temp/RH).
2. ESP32 published per MQTT in dein Netzwerk.
3. PC-Skript `co2.py` subscribed auf die MQTT-Themen, wertet aus und zeigt die Daten im Overlay an.

---

## MQTT Topics & Payload (aktuell im Python-Code)

### CO₂
- **Topic:** `sensor/co2`
- **Payload (JSON):** erwartet mindestens:
  ```json
  { "eco2": 624 }
  ```

### Stromzähler / Leistung (optional)
- **Topic:** `sensor/stromzaehler/SENSOR`
- **Payload (JSON):** erwartet:
  ```json
  { "GS303": { "Power_cur": 50 } }
  ```

Wenn du andere Topics/Keys nutzt, musst du sie in `on_message()` anpassen.

---

## Installation (PC / Overlay)

### Voraussetzungen
- Windows (durch `co2.bat` so vorgesehen; `co2.py` läuft grundsätzlich auch auf anderen OS mit Anpassungen)
- Python 3.x
- MQTT Broker erreichbar (z. B. Mosquitto auf NAS/RPi/Server)

### Python Dependencies
Installiere:
```bash
pip install paho-mqtt
```

(Tkinter ist bei vielen Python-Installationen bereits dabei.)

---

## Starten

### Direkt per Python
Passe in `co2.py` die Broker-IP an:
```python
client.connect("192.168.178.151")
```

Dann starten:
```bash
python co2.py
```

### Start per `co2.bat` (Autostart/Shortcut)
Die BAT startet `pythonw.exe` minimiert (ohne Console).  
**Wichtig:** Die Pfade sind aktuell hart codiert und müssen auf deinem PC angepasst werden.

---

## Konfiguration im Code (co2.py)

### CO₂-Ampel-Schwellenwerte
```python
if eco2_value >= 1100:
    co2_color = 'red'
elif eco2_value >= 800:
    co2_color = 'orange'
```

### Verlauf / Diagramm
Aktuell:
- `power_history = [0] * 48` → 48 Werte
- Kommentar sagt „10min-Takt“, im Code wird aber **bei jedem MQTT-Power-Update** ein Wert angehängt.

Wenn du wirklich „alle 10 Minuten einen Punkt“ willst, brauchst du entweder:
- ein Timer-Sampling im Client, oder
- serverseitige Aggregation (z. B. Node-RED/Telegraf/Influx).

---

## Projektstruktur (empfohlen)

```
.
├─ co2.py
├─ co2.bat
└─ docs/
   ├─ DOCUMENTATION.md
   └─ images/
      └─ setup.png
```

---

## Roadmap / Ideen

- ESP32/C++ Firmware hinzufügen (Sensor-Auslesen, Display, MQTT)
- Auto-Reconnect / Heartbeat stabiler machen
- Logging (CSV/InfluxDB)
- Dynamisches Scaling im Diagramm
- Konfigurationsdatei (`config.json`) für Broker/Topics/Thresholds

---

## Lizenz
Noch keine Lizenz festgelegt. (Optional: MIT, Apache-2.0, GPL-3.0, …)