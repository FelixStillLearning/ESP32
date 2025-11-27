# ESP32 Smart Home Main Controller

## 📋 Deskripsi
Controller utama untuk sistem Smart Home yang menghubungkan berbagai sensor dan aktuator ke backend melalui MQTT.

## 🔧 Hardware Requirements

### ESP32 Development Board
- ESP32 DevKit V1 atau equivalent
- Pastikan memiliki WiFi built-in

### Sensors
| Sensor | Model | GPIO Pin | Tipe |
|--------|-------|----------|------|
| Temperature & Humidity | DHT11/DHT22 | GPIO 13 | Digital |
| Light Sensor | LDR | GPIO 12 | Analog |
| Gas Sensor | MQ-2 | GPIO 14 | Analog |

### Actuators & Display
| Device | GPIO Pin | Notes |
|--------|----------|-------|
| Relay Module | GPIO 27 (IN1) | Active LOW |
| LCD I2C | SDA=GPIO21, SCL=GPIO22 | Address: 0x27 |

### Keypad 4x4
| Pin | GPIO |
|-----|------|
| ROW1 | GPIO 15 |
| ROW2 | GPIO 2 |
| ROW3 | GPIO 0 |
| ROW4 | GPIO 4 |
| COL1 | GPIO 16 |
| COL2 | GPIO 17 |
| COL3 | GPIO 5 |
| COL4 | GPIO 18 |

## 📐 Wiring Diagram

```
                    ┌─────────────────────────────────────┐
                    │            ESP32 DevKit             │
                    │                                     │
    ┌───────────┐   │  GPIO13 ◄──── DHT DATA             │
    │   DHT11   │───┤  3.3V   ◄──── DHT VCC              │
    │           │   │  GND    ◄──── DHT GND              │
    └───────────┘   │                                     │
                    │  GPIO12 ◄──── LDR (with voltage    │
    ┌───────────┐   │                divider)            │
    │    LDR    │───┤  3.3V   ◄──── LDR VCC              │
    │           │   │  GND    ◄──── LDR GND              │
    └───────────┘   │                                     │
                    │  GPIO14 ◄──── MQ-2 AO              │
    ┌───────────┐   │  5V     ◄──── MQ-2 VCC             │
    │   MQ-2    │───┤  GND    ◄──── MQ-2 GND             │
    │           │   │                                     │
    └───────────┘   │  GPIO27 ────► RELAY IN1            │
                    │  5V     ────► RELAY VCC            │
    ┌───────────┐   │  GND    ────► RELAY GND            │
    │   RELAY   │───┤                                     │
    │           │   │  GPIO21 ────► LCD SDA              │
    └───────────┘   │  GPIO22 ────► LCD SCL              │
                    │  5V     ────► LCD VCC              │
    ┌───────────┐   │  GND    ────► LCD GND              │
    │  LCD I2C  │───┤                                     │
    │           │   │                                     │
    └───────────┘   │  GPIO 15,2,0,4,16,17,5,18          │
                    │         ▲                           │
    ┌───────────┐   │         │                           │
    │  KEYPAD   │───┴─────────┘                           │
    │   4x4     │                                         │
    └───────────┘                                         │
                    └─────────────────────────────────────┘
```

## 📚 Libraries Required

Install melalui Arduino Library Manager:

1. **PubSubClient** - by Nick O'Leary (MQTT Client)
2. **DHT sensor library** - by Adafruit
3. **Adafruit Unified Sensor** - by Adafruit
4. **LiquidCrystal_I2C** - by Frank de Brabander
5. **Keypad** - by Mark Stanley, Alexander Brevig
6. **ArduinoJson** - by Benoit Blanchon

## ⚙️ Configuration

Edit bagian ini di file `.ino`:

```cpp
// WiFi Configuration
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// MQTT Configuration
const char* MQTT_SERVER = "192.168.1.100";  // IP Backend Server
const int MQTT_PORT = 1883;

// PIN Code
const String CORRECT_PIN = "1234";  // Ubah sesuai kebutuhan
```

## 📡 MQTT Topics

### Publish (ESP32 → Server)
| Topic | Payload | Interval |
|-------|---------|----------|
| `home/temperature` | `{"temperature": 25.5, "unit": "celsius"}` | 5 detik |
| `home/humidity` | `{"humidity": 60.0, "unit": "percent"}` | 5 detik |
| `home/gas` | `{"ppm": 150}` | 5 detik |
| `home/light` | `{"lux": 500}` | 5 detik |
| `home/lamp/status` | `{"status": "on", "mode": "manual"}` | On change |
| `home/door/status` | `{"status": "locked", "method": "keypad"}` | On change |

### Subscribe (Server → ESP32)
| Topic | Payload Example |
|-------|-----------------|
| `home/lamp/control` | `{"action": "on", "mode": "auto"}` |
| `home/door/control` | `{"action": "unlock", "method": "remote"}` |

## 🎮 Keypad Functions

| Key | Function |
|-----|----------|
| 0-9 | Input PIN |
| * | Clear PIN |
| # | Submit PIN |
| A | Toggle Lamp ON/OFF |
| B | Cycle LCD Display |
| C | Reserved |
| D | Reserved |

## 🖥️ LCD Display Modes

LCD akan berganti tampilan setiap 3 detik:

1. **Mode 0**: Temperature & Humidity
2. **Mode 1**: Gas & Light levels
3. **Mode 2**: Lamp & Door status
4. **Mode 3**: WiFi & MQTT connection status

## 🔌 Upload Instructions

1. Buka Arduino IDE
2. Pilih Board: `ESP32 Dev Module`
3. Pilih Port yang sesuai
4. Pastikan semua library sudah terinstall
5. Upload code

## 🐛 Troubleshooting

### WiFi tidak connect
- Periksa SSID dan password
- Pastikan ESP32 dalam jangkauan WiFi
- Coba restart ESP32

### MQTT tidak connect
- Pastikan MQTT broker sudah running
- Periksa IP address dan port
- Check firewall settings

### Sensor tidak terbaca
- Periksa wiring
- Pastikan VCC dan GND terhubung dengan benar
- Cek nilai GPIO pin

### LCD tidak menyala
- Periksa alamat I2C (0x27 atau 0x3F)
- Atur kontras dengan potentiometer di belakang LCD
- Periksa koneksi SDA dan SCL

## 📊 Serial Monitor Output

Buka Serial Monitor dengan baud rate `115200` untuk melihat debug output:

```
========================================
ESP32 Smart Home Controller Starting...
========================================
Connecting to WiFi: FELIXTHE3RD
........
✅ WiFi Connected!
IP Address: 192.168.1.105
Connecting to MQTT...✅ Connected!
Subscribed to:
  - home/lamp/control
  - home/door/control
✅ System initialized successfully!
📊 Sensor Readings:
  Temperature: 26.5°C
  Humidity: 65.0%
  Gas (PPM): 120
  Light (Lux): 450
📤 Published temperature: {"temperature":26.5,"unit":"celsius"}
📤 Published humidity: {"humidity":65.0,"unit":"percent"}
📤 Published gas: {"ppm":120}
📤 Published light: {"lux":450}
```
