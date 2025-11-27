# 🏠 Smart Home ESP32 IoT System

Repository untuk semua kode ESP32 dalam sistem Smart Home IoT.

## 📁 Struktur Folder

```
ESP32/
├── esp32_main_controller/     # Controller utama (sensor & aktuator)
│   ├── esp32_main_controller.ino
│   └── README.md
└── esp32_cam_face_recognition/ # ESP32-CAM untuk face recognition
    └── esp32cam/
        ├── esp32cam.ino
        ├── handlers.cpp
        ├── WifiCam.hpp
        └── README.md
```

## 🔧 Hardware Requirements

### ESP32 Main Controller
- **Board**: ESP32 DevKit V1 / ESP32 WROOM-32
- **Sensors**:
  - DHT22 (Temperature & Humidity)
  - MQ-2 (Gas Sensor)
  - LDR (Light Sensor)
- **Actuators**:
  - Relay Module (4 Channel)
  - Servo Motor (Curtain control)
  - Buzzer
- **Other**:
  - Fingerprint Sensor (R307/AS608)
  - LED indicators

### ESP32-CAM
- **Board**: ESP32-CAM (AI-Thinker)
- **Camera**: OV2640
- **FTDI Programmer** (untuk upload)

## 📦 Library Dependencies

Install library berikut melalui Arduino IDE Library Manager:

### Main Controller
```
- WiFi (built-in)
- PubSubClient by Nick O'Leary
- DHT sensor library by Adafruit
- Adafruit Unified Sensor
- ArduinoJson by Benoit Blanchon
- ESP32Servo by Kevin Harrington
- Adafruit Fingerprint Sensor Library
```

### ESP32-CAM
```
- WiFi (built-in)
- esp32-camera (built-in)
- ArduinoWebsockets by Gil Maimon (opsional)
```

## ⚙️ Konfigurasi

### 1. WiFi Configuration

Edit di masing-masing file `.ino`:

```cpp
// WiFi credentials
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASS = "Your_WiFi_Password";
```

### 2. MQTT Broker Configuration

**Pilihan A: Broker Lokal (Mosquitto)**
```cpp
const char* MQTT_SERVER = "192.168.1.100";  // IP komputer Laragon
const int MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASS = "";
```

**Pilihan B: Broker Publik (HiveMQ)**
```cpp
const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASS = "";
```

### 3. Pin Configuration

Pin assignment sudah didefinisikan di masing-masing file. Lihat detail di:
- [`esp32_main_controller/README.md`](esp32_main_controller/README.md)

## 🚀 Upload ke ESP32

### Main Controller

1. Buka `esp32_main_controller/esp32_main_controller.ino` di Arduino IDE
2. Pilih Board: **ESP32 Dev Module**
3. Pilih Port: **COM port ESP32**
4. Upload Speed: **115200**
5. Flash Frequency: **80MHz**
6. Partition Scheme: **Default 4MB**
7. Klik **Upload** ⬆️

### ESP32-CAM

1. Sambungkan **FTDI ke ESP32-CAM**:
   ```
   FTDI TX  → ESP32-CAM RX
   FTDI RX  → ESP32-CAM TX
   FTDI GND → ESP32-CAM GND
   FTDI 5V  → ESP32-CAM 5V
   GPIO 0   → GND (saat upload)
   ```
2. Buka `esp32_cam_face_recognition/esp32cam/esp32cam.ino`
3. Pilih Board: **AI Thinker ESP32-CAM**
4. Upload Speed: **115200**
5. Klik **Upload**
6. Setelah upload, lepas **GPIO 0 dari GND**
7. Tekan **RST button** untuk restart

## 📊 MQTT Topics

### Published Topics (ESP32 → Backend)

| Topic | Description | Payload Format |
|-------|-------------|----------------|
| `smart_home/sensor/temperature` | Data suhu | `{"temperature": 25.5}` |
| `smart_home/sensor/humidity` | Data kelembaban | `{"humidity": 60.2}` |
| `smart_home/sensor/gas` | Data gas | `{"gas_level": 120}` |
| `smart_home/sensor/light` | Data cahaya | `{"light_level": 450}` |
| `smart_home/door/status` | Status pintu | `{"status": "open"}` |
| `smart_home/device/status` | Status device | `{"device": "lamp1", "state": "on"}` |

### Subscribed Topics (Backend → ESP32)

| Topic | Description | Payload Format |
|-------|-------------|----------------|
| `smart_home/control/lamp` | Kontrol lampu | `{"lamp": "lamp1", "state": "on"}` |
| `smart_home/control/curtain` | Kontrol tirai | `{"action": "open"}` |
| `smart_home/control/buzzer` | Kontrol buzzer | `{"state": "on", "duration": 1000}` |
| `smart_home/control/door` | Kontrol pintu | `{"action": "unlock"}` |

## 🧪 Testing

### 1. Cek Koneksi WiFi
```
Buka Serial Monitor (115200 baud)
Harusnya muncul:
✅ WiFi connected!
✅ IP Address: 192.168.x.x
```

### 2. Cek Koneksi MQTT
```
✅ MQTT connected!
📤 Publishing to: smart_home/sensor/temperature
```

### 3. Test Sensor
```
🌡️  Temperature: 25.5 °C
💧 Humidity: 60.2 %
💨 Gas Level: 120 ppm
💡 Light Level: 450 lux
```

### 4. Test Control
Gunakan MQTT client (MQTT Explorer / mqttx):
- Publish ke `smart_home/control/lamp`
- Payload: `{"lamp": "lamp1", "state": "on"}`
- Cek LED menyala

## 🔗 Related Repositories

- **Backend Go**: [backend_GO_iot](../backend_go) - REST API & MQTT Handler
- **Flutter App**: [smart_home_flutter](../smart_home_flutter) - Mobile App
- **Python Face Service**: [python_face_service](../python_face_service) - Face Recognition

## 📝 Troubleshooting

### ESP32 tidak bisa connect ke WiFi
```cpp
// Coba restart ESP32
// Cek SSID & password
// Pastikan WiFi 2.4GHz (bukan 5GHz)
```

### MQTT tidak connect
```cpp
// Cek IP broker sudah benar
// Cek firewall tidak block port 1883
// Cek broker service sudah running
```

### Sensor tidak terbaca
```cpp
// Cek wiring/kabel
// Cek pin assignment
// Cek sensor sudah dapat power
```

## 📄 License

MIT License - Free to use and modify

## 👨‍💻 Author

**Kysu-dev**
- Repository: [backend_GO_iot](https://github.com/Kysu-dev/backend_GO_iot)

---

**Made with ❤️ for Smart Home IoT Project**
