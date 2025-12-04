#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

// ==================== WiFi Configuration ====================
const char* WIFI_SSID = "FELIXTHE3RD";
const char* WIFI_PASS = "superadmin";

// ==================== MQTT Configuration ====================
const char* MQTT_SERVER = "broker.hivemq.com"; 
const int MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASS = "";

// MQTT Topics - Publish (Sensor Data)
const char* TOPIC_TEMPERATURE = "iotcihuy/home/temperature";
const char* TOPIC_HUMIDITY = "iotcihuy/home/humidity";
const char* TOPIC_GAS = "iotcihuy/home/gas";
const char* TOPIC_LIGHT = "iotcihuy/home/light";

// MQTT Topics - Subscribe (Control Commands)
const char* TOPIC_LAMP_CONTROL = "iotcihuy/home/lamp/control";
const char* TOPIC_DOOR_CONTROL = "iotcihuy/home/door/control";
const char* TOPIC_CURTAIN_CONTROL = "iotcihuy/home/curtain/control";

// MQTT Topics - Status (Feedback to server)
const char* TOPIC_LAMP_STATUS = "iotcihuy/home/lamp/status";
const char* TOPIC_DOOR_STATUS = "iotcihuy/home/door/status";
const char* TOPIC_CURTAIN_STATUS = "iotcihuy/home/curtain/status";

// ==================== Pin Definitions ====================
// DHT Sensor 
#define DHT_PIN 13
#define DHT_TYPE DHT22

// Analog Sensors (ADC1 pins - safe for WiFi)
#define LDR_PIN 32   
#define MQ2_PIN 35   

// Relay Module (2 Channel)
#define RELAY_LAMP_PIN 27
#define RELAY_DOOR_PIN 26

// Servo (Curtain/Gorden)
#define SERVO_PIN 25

// Buzzer (Gas Alert)
#define BUZZER_PIN 23

// DFPlayer Mini (Voice Module)
#define DFPLAYER_RX 16  // ESP32 RX ← DFPlayer TX
#define DFPLAYER_TX 17  // ESP32 TX → DFPlayer RX

// LCD I2C (SDA=21, SCL=22 - default)
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Keypad Configuration (Fixed pins - avoid bootstrap)
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {19, 5, 15, 12};  // Row 1-4
byte colPins[COLS] = {18, 4, 27, 14};   // Col 1-4

// ==================== Objects ====================
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo curtainServo;
HardwareSerial dfSerial(2);  // UART2
DFRobotDFPlayerMini dfPlayer;

// ==================== Variables ====================
float temperature = 0;
float humidity = 0;
int gasValue = 0;
int lightValue = 0;

bool lampState = false;
String lampMode = "manual";
bool doorLocked = true;
bool curtainOpen = false;
int curtainPosition = 0;  // 0=closed, 180=open

unsigned long lastSensorRead = 0;
unsigned long lastMQTTPublish = 0;
const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long MQTT_INTERVAL = 5000;

String enteredPin = "";
const String CORRECT_PIN = "1234";
int mq2Baseline = 0;
bool mq2Calibrated = false;

const int GAS_ALERT_THRESHOLD = 300;  // PPM threshold for buzzer
bool gasAlertActive = false;

unsigned long lcdMessageTimer = 0;
bool showingMessage = false;

// Door auto-lock timer
unsigned long doorUnlockTimer = 0;
bool doorAutoLockPending = false;
const unsigned long DOOR_UNLOCK_DURATION = 5000;  // 5 seconds

bool dfPlayerReady = false;

// DFPlayer Voice Track Numbers (save as 0001.mp3, 0002.mp3, etc. on SD card)
#define VOICE_WELCOME 1         // "Selamat datang"
#define VOICE_DOOR_UNLOCKED 2   // "Pintu terbuka"
#define VOICE_DOOR_LOCKED 3     // "Pintu terkunci"
#define VOICE_WRONG_PIN 4       // "PIN salah, coba lagi"
#define VOICE_CORRECT_PIN 5     // "PIN benar"
#define VOICE_GAS_ALERT 6       // "Peringatan! Gas terdeteksi!"
#define VOICE_FACE_RECOGNIZED 7 // "Wajah dikenali, selamat datang"
#define VOICE_FACE_UNKNOWN 8    // "Wajah tidak dikenali"

// ==================== Function Prototypes ====================
void setupWiFi();
void setupMQTT();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void readSensors();
void publishSensorData();
void handleKeypad();
void updateLCD();
void controlLamp(bool state, String mode);
void controlDoor(bool lock, String method);
void controlCurtain(bool open);
void playVoice(int trackNumber);
void checkGasAlert();
void soundBuzzer(int count);
void checkDoorAutoLock();

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("ESP32 Smart Home Controller Starting...");
  Serial.println("========================================");

  // Configure output pins
  pinMode(RELAY_LAMP_PIN, OUTPUT);
  pinMode(RELAY_DOOR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initial state (Relay active LOW)
  digitalWrite(RELAY_LAMP_PIN, HIGH);  // Lamp OFF
  digitalWrite(RELAY_DOOR_PIN, HIGH);  // Door LOCKED
  digitalWrite(BUZZER_PIN, LOW);       // Buzzer OFF

  // Initialize Servo
  curtainServo.attach(SERVO_PIN);
  curtainServo.write(0);  // Closed position
  Serial.println("[OK] Servo initialized");

  // Initialize I2C & LCD
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  Serial.println("[OK] LCD initialized");

  // Initialize DFPlayer
  dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  delay(1000);
  
  if (dfPlayer.begin(dfSerial)) {
    dfPlayerReady = true;
    dfPlayer.volume(25);  // 0-30
    Serial.println("[OK] DFPlayer initialized");
  } else {
    dfPlayerReady = false;
    Serial.println("[WARN] DFPlayer not found - voice disabled");
  }

  // Initialize DHT
  dht.begin();
  Serial.println("[OK] DHT22 initialized");
  delay(2000);

  // Calibrate MQ-2
  lcd.setCursor(0, 1);
  lcd.print("Calibrating...  ");
  Serial.println("[...] Calibrating MQ-2 sensor...");
  
  long sum = 0;
  int samples = 20;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(MQ2_PIN);
    delay(200);
  }
  mq2Baseline = sum / samples;
  mq2Calibrated = true;
  Serial.printf("[OK] MQ-2 Baseline: %d ADC\n", mq2Baseline);

  // Setup WiFi & MQTT
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi ");
  setupWiFi();
  setupMQTT();

  // Ready
  Serial.println("\n========================================");
  Serial.println("System Ready!");
  Serial.println("========================================");
  Serial.println("Keypad Shortcuts:");
  Serial.println("  A = Toggle Lamp");
  Serial.println("  B = Toggle Curtain");
  Serial.println("  C = Toggle Door Lock");
  Serial.println("  # = Submit PIN");
  Serial.println("  * = Clear PIN");
  Serial.println("========================================\n");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Door: LOCKED");
  lcd.setCursor(0, 1);
  lcd.print("Enter PIN");

  // Welcome beep
  soundBuzzer(1);
  playVoice(VOICE_WELCOME);
}

// ==================== Main Loop ====================
void loop() {
  if (!mqtt.connected()) {
    reconnectMQTT();
  }
  mqtt.loop();

  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    readSensors();
    checkGasAlert();
    lastSensorRead = millis();
  }

  if (millis() - lastMQTTPublish >= MQTT_INTERVAL) {
    publishSensorData();
    lastMQTTPublish = millis();
  }

  handleKeypad();
  
  // Check door auto-lock timer
  checkDoorAutoLock();
  
  // Auto-clear LCD message after 3 seconds
  if (showingMessage && (millis() - lcdMessageTimer >= 3000)) {
    showingMessage = false;
    updateLCD();
  }
}

// ==================== WiFi Setup ====================
void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[FAIL] WiFi Connection Failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi FAILED!");
    lcd.setCursor(0, 1);
    lcd.print("Restarting...");
    delay(3000);
    ESP.restart();
  }
}

// ==================== MQTT Setup ====================
void setupMQTT() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);
}

void reconnectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");

    String clientId = "ESP32-SmartHome-" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println(" Connected!");

      mqtt.subscribe(TOPIC_LAMP_CONTROL);
      mqtt.subscribe(TOPIC_DOOR_CONTROL);
      mqtt.subscribe(TOPIC_CURTAIN_CONTROL);

      Serial.println("Subscribed to control topics");
    } else {
      Serial.print(" Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Retry in 5s...");
      delay(5000);
    }
  }
}

// ==================== MQTT Callback ====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.printf("[MQTT] %s: %s\n", topic, message.c_str());

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.println("[MQTT] JSON parse failed!");
    return;
  }

  String topicStr = String(topic);

  // Lamp Control
  if (topicStr == TOPIC_LAMP_CONTROL) {
    String action = doc["action"] | "";
    String mode = doc["mode"] | "manual";

    if (action == "on") {
      controlLamp(true, mode);
    } else if (action == "off") {
      controlLamp(false, mode);
    } else if (action == "toggle") {
      controlLamp(!lampState, mode);
    }
  }

  // Door Control
  if (topicStr == TOPIC_DOOR_CONTROL) {
    String action = doc["action"] | "";
    String method = doc["method"] | "remote";
    String username = doc["username"] | "";

    if (action == "lock") {
      controlDoor(true, method);
    } else if (action == "unlock") {
      controlDoor(false, method);
      // If face recognition unlock
      if (method == "face" && username != "") {
        playVoice(VOICE_FACE_RECOGNIZED);
      }
    }
  }

  // Curtain Control
  if (topicStr == TOPIC_CURTAIN_CONTROL) {
    String action = doc["action"] | "";

    if (action == "open") {
      controlCurtain(true);
    } else if (action == "close") {
      controlCurtain(false);
    } else if (action == "toggle") {
      controlCurtain(!curtainOpen);
    }
  }
}

// ==================== Sensor Reading ====================
void readSensors() {
  // Read DHT22
  float newTemp = dht.readTemperature();
  float newHumid = dht.readHumidity();

  if (!isnan(newTemp) && !isnan(newHumid)) {
    temperature = newTemp;
    humidity = newHumid;
  }

  // Read MQ-2 (Gas Sensor)
  int mq2Raw = analogRead(MQ2_PIN);
  
  if (mq2Calibrated) {
    int difference = mq2Raw - mq2Baseline;
    if (difference < 0) difference = 0;
    
    const int MQ2_THRESHOLD = 100;
    
    if (difference < MQ2_THRESHOLD) {
      gasValue = 0;
    } else {
      gasValue = constrain(map(difference, MQ2_THRESHOLD, 1500, 0, 1000), 0, 1000);
    }
  } else {
    gasValue = constrain(map(mq2Raw, 0, 4095, 0, 1000), 0, 1000);
  }
  
  // Read LDR (Light Sensor)
  int ldrRaw = analogRead(LDR_PIN);
  lightValue = 1000 - constrain(map(ldrRaw, 0, 4095, 0, 1000), 0, 1000);

  // Debug output
  Serial.printf("[Sensors] T:%.1f°C H:%.1f%% Gas:%dPPM Light:%dLux\n", 
                temperature, humidity, gasValue, lightValue);
}

// ==================== Gas Alert ====================
void checkGasAlert() {
  if (gasValue > GAS_ALERT_THRESHOLD && !gasAlertActive) {
    gasAlertActive = true;
    Serial.println("[ALERT] Gas level HIGH!");
    
    playVoice(VOICE_GAS_ALERT);
    soundBuzzer(5);  // 5 beeps
    
    // Show on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("!! GAS ALERT !!");
    lcd.setCursor(0, 1);
    lcd.print("Level: ");
    lcd.print(gasValue);
    lcd.print(" PPM");
    
    showingMessage = true;
    lcdMessageTimer = millis();
    
  } else if (gasValue <= GAS_ALERT_THRESHOLD - 50 && gasAlertActive) {
    // Hysteresis to prevent flapping
    gasAlertActive = false;
    Serial.println("[OK] Gas level normal");
  }
}

// ==================== MQTT Publish ====================
void publishSensorData() {
  if (!mqtt.connected()) return;

  StaticJsonDocument<128> doc;
  char buffer[128];

  // Publish Temperature
  doc.clear();
  doc["temperature"] = temperature;
  doc["unit"] = "celsius";
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_TEMPERATURE, buffer);

  // Publish Humidity
  doc.clear();
  doc["humidity"] = humidity;
  doc["unit"] = "percent";
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_HUMIDITY, buffer);

  // Publish Gas
  doc.clear();
  doc["ppm"] = gasValue;
  doc["alert"] = gasAlertActive;
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_GAS, buffer);

  // Publish Light
  doc.clear();
  doc["lux"] = lightValue;
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_LIGHT, buffer);

  Serial.println("[MQTT] Published sensor data");
}

// ==================== Device Control ====================
void controlLamp(bool state, String mode) {
  lampState = state;
  lampMode = mode;

  digitalWrite(RELAY_LAMP_PIN, state ? LOW : HIGH);

  Serial.printf("[Lamp] %s (%s mode)\n", state ? "ON" : "OFF", mode.c_str());

  StaticJsonDocument<128> doc;
  char buffer[128];
  doc["status"] = state ? "on" : "off";
  doc["mode"] = mode;
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_LAMP_STATUS, buffer);
}

void controlDoor(bool lock, String method) {
  doorLocked = lock;

  // Solenoid: LOW = unlock (energized), HIGH = lock (de-energized)
  digitalWrite(RELAY_DOOR_PIN, lock ? HIGH : LOW);

  Serial.printf("[Door] %s (%s)\n", lock ? "LOCKED" : "UNLOCKED", method.c_str());

  // Play voice
  if (lock) {
    playVoice(VOICE_DOOR_LOCKED);
    doorAutoLockPending = false;  // Cancel any pending auto-lock
  } else {
    playVoice(VOICE_DOOR_UNLOCKED);
    // Start auto-lock timer (5 seconds)
    doorUnlockTimer = millis();
    doorAutoLockPending = true;
    Serial.println("[Door] Auto-lock in 5 seconds...");
  }

  StaticJsonDocument<128> doc;
  char buffer[128];
  doc["status"] = lock ? "locked" : "unlocked";
  doc["method"] = method;
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_DOOR_STATUS, buffer);

  updateLCD();
}

// ==================== Door Auto-Lock ====================
void checkDoorAutoLock() {
  if (doorAutoLockPending && !doorLocked) {
    if (millis() - doorUnlockTimer >= DOOR_UNLOCK_DURATION) {
      Serial.println("[Door] Auto-locking now!");
      controlDoor(true, "auto");
      
      // Show on LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("AUTO LOCKED");
      lcd.setCursor(0, 1);
      lcd.print("Door secured");
      
      showingMessage = true;
      lcdMessageTimer = millis();
    }
  }
}

void controlCurtain(bool open) {
  curtainOpen = open;
  int targetPos = open ? 180 : 0;
  
  Serial.printf("[Curtain] Moving to %s...\n", open ? "OPEN" : "CLOSED");
  
  // Smooth movement
  int step = (targetPos > curtainPosition) ? 2 : -2;
  
  while (curtainPosition != targetPos) {
    curtainPosition += step;
    if ((step > 0 && curtainPosition > targetPos) || 
        (step < 0 && curtainPosition < targetPos)) {
      curtainPosition = targetPos;
    }
    curtainServo.write(curtainPosition);
    delay(15);
  }
  
  Serial.printf("[Curtain] %s (pos: %d)\n", open ? "OPEN" : "CLOSED", curtainPosition);

  StaticJsonDocument<128> doc;
  char buffer[128];
  doc["status"] = open ? "open" : "closed";
  doc["position"] = curtainPosition;
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_CURTAIN_STATUS, buffer);
}

void playVoice(int trackNumber) {
  if (dfPlayerReady) {
    dfPlayer.play(trackNumber);
    Serial.printf("[Voice] Playing track %d\n", trackNumber);
  }
}

void soundBuzzer(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

// ==================== Keypad Handling ====================
void handleKeypad() {
  char key = keypad.getKey();

  if (key) {
    Serial.printf("[Keypad] Key: %c\n", key);

    if (key == '#') {
      // Submit PIN
      if (enteredPin.length() == 0) {
        // No PIN entered, just show prompt
        return;
      }
      
      if (enteredPin == CORRECT_PIN) {
        Serial.println("[PIN] Correct!");
        soundBuzzer(1);  // Success beep
        playVoice(VOICE_CORRECT_PIN);
        

        controlDoor(false, "pin"); 

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("PIN CORRECT!");
        lcd.setCursor(0, 1);
        lcd.print("Door UNLOCKED");

        showingMessage = true;
        lcdMessageTimer = millis();

      } else {
        Serial.println("[PIN] Incorrect!");
        soundBuzzer(3);  // Error beeps
        playVoice(VOICE_WRONG_PIN);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WRONG PIN!");
        lcd.setCursor(0, 1);
        lcd.print("Try Again");

        showingMessage = true;
        lcdMessageTimer = millis();
      }
      enteredPin = "";

    } else if (key == '*') {
      // Clear PIN
      enteredPin = "";
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PIN Cleared");
      lcd.setCursor(0, 1);
      lcd.print("Enter PIN");
      
      showingMessage = true;
      lcdMessageTimer = millis();

    } else if (key >= '0' && key <= '9') {
      // PIN digit input
      if (enteredPin.length() < 6) {
        enteredPin += key;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter PIN:");
        lcd.setCursor(0, 1);

        // Show stars for entered digits
        for (unsigned int i = 0; i < enteredPin.length(); i++) {
          lcd.print("*");
        }
        showingMessage = false;
      }
    }
    // Keys A, B, C, D are ignored (control via mobile/web only)
  }
}

// ==================== LCD Update ====================
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Door: ");
  lcd.print(doorLocked ? "LOCKED" : "UNLOCKED");
  lcd.setCursor(0, 1);
  lcd.print("Enter PIN");
}