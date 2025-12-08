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

// MQTT Topics
const char* TOPIC_TEMPERATURE = "iotcihuy/home/temperature";
const char* TOPIC_HUMIDITY = "iotcihuy/home/humidity";
const char* TOPIC_GAS = "iotcihuy/home/gas";
const char* TOPIC_LIGHT = "iotcihuy/home/light";

const char* TOPIC_LAMP_CONTROL = "iotcihuy/home/lamp/control";
const char* TOPIC_DOOR_CONTROL = "iotcihuy/home/door/control";
const char* TOPIC_CURTAIN_CONTROL = "iotcihuy/home/curtain/control";
const char* TOPIC_BUZZER_CONTROL = "iotcihuy/home/buzzer/control";

const char* TOPIC_LAMP_STATUS = "iotcihuy/home/lamp/status";
const char* TOPIC_DOOR_STATUS = "iotcihuy/home/door/status";
const char* TOPIC_CURTAIN_STATUS = "iotcihuy/home/curtain/status";
const char* TOPIC_FACE_RECOGNITION = "iotcihuy/home/face/recognition";

// ==================== Pin Definitions ====================
#define DHT_PIN 13
#define DHT_TYPE DHT22

#define LDR_PIN 32   
#define MQ2_PIN 35   

// --- PERBAIKAN PIN RELAY LAMPU (Agar tidak bentrok dengan Keypad) ---
#define RELAY_LAMP_PIN 33  // Pindah ke GPIO 33
#define RELAY_DOOR_PIN 26 

#define SERVO_PIN 25
#define BUZZER_PIN 23

// DFPlayer Mini Pins
#define DFPLAYER_RX 16  
#define DFPLAYER_TX 17  

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {19, 5, 15, 12};
byte colPins[COLS] = {18, 4, 27, 14}; // Pin 27 dipakai disini

// ==================== Objects ====================
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo curtainServo;
HardwareSerial dfSerial(2); 
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
int curtainPosition = 0; 

unsigned long lastSensorRead = 0;
unsigned long lastMQTTPublish = 0;
const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long MQTT_INTERVAL = 5000;

String enteredPin = "";
const String CORRECT_PIN = "1234";
int mq2Baseline = 0;
bool mq2Calibrated = false;

// PIN Timeout (5 minutes)
unsigned long pinEntryStartTime = 0;
bool pinEntryActive = false;
const unsigned long PIN_TIMEOUT = 300000; // 5 minutes

// Face Recognition State
bool faceRecognitionMode = false;
unsigned long faceRecognitionTimer = 0;
const unsigned long FACE_RECOGNITION_TIMEOUT = 30000; // 30 seconds

// Buzzer (Backend Control)
bool buzzerState = false;
unsigned long lastBuzzerToggle = 0;
const unsigned long BUZZER_BEEP_INTERVAL = 200;

unsigned long lcdMessageTimer = 0;
bool showingMessage = false;

unsigned long doorUnlockTimer = 0;
bool doorAutoLockPending = false;
const unsigned long DOOR_UNLOCK_DURATION = 5000;

bool dfPlayerReady = false;

// Voice Tracks
#define VOICE_WELCOME 1         // "Selamat datang"
#define VOICE_DOOR_UNLOCKED 2   // "Pintu terbuka"
#define VOICE_DOOR_LOCKED 3     // "Pintu terkunci"
#define VOICE_WRONG_PIN 4       // "PIN salah"
#define VOICE_CORRECT_PIN 5     // "PIN benar"
#define VOICE_GAS_ALERT 6       // "Gas bahaya"
#define VOICE_FACE_DENIED 7     // "Wajah tidak dikenali"

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
void controlBuzzer(bool state);
void playVoice(int trackNumber);
void checkDoorAutoLock();
void checkPinTimeout();
void startFaceRecognition();

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_LAMP_PIN, OUTPUT);
  pinMode(RELAY_DOOR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  digitalWrite(RELAY_LAMP_PIN, HIGH); 
  digitalWrite(RELAY_DOOR_PIN, HIGH); 
  digitalWrite(BUZZER_PIN, LOW);      

  curtainServo.attach(SERVO_PIN);
  curtainServo.write(0);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.print("System Starting");

  dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  delay(1000);
  
  if (dfPlayer.begin(dfSerial)) {
    dfPlayerReady = true;
    dfPlayer.volume(25);
    Serial.println("[OK] DFPlayer Ready");
    playVoice(VOICE_WELCOME); 
  } else {
    dfPlayerReady = false;
    Serial.println("[WARN] DFPlayer Not Found");
  }

  dht.begin();
  delay(2000);

  lcd.setCursor(0, 1);
  lcd.print("Calibrating Gas");
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(MQ2_PIN);
    delay(100);
  }
  mq2Baseline = sum / 20;
  mq2Calibrated = true;

  lcd.clear(); lcd.print("Connecting WiFi");
  setupWiFi();
  setupMQTT();

  lcd.clear();
  lcd.print("Door: LOCKED");
  lcd.setCursor(0, 1);
  lcd.print("Enter PIN");
}

// ==================== Main Loop ====================
void loop() {
  if (WiFi.status() == WL_CONNECTED && !mqtt.connected()) {
    reconnectMQTT();
  }
  mqtt.loop();

  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    readSensors();
    lastSensorRead = millis();
  }

  if (millis() - lastMQTTPublish >= MQTT_INTERVAL) {
    publishSensorData();
    lastMQTTPublish = millis();
  }

  handleKeypad();
  checkDoorAutoLock();
  checkPinTimeout();
  
  if (showingMessage && (millis() - lcdMessageTimer >= 3000)) {
    showingMessage = false;
    updateLCD();
  }

  if (buzzerState) {
    if (millis() - lastBuzzerToggle >= BUZZER_BEEP_INTERVAL) {
      digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN)); 
      lastBuzzerToggle = millis();
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW); 
  }
}

// ==================== WiFi & MQTT ====================
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
    delay(500); Serial.print("."); 
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi Connected");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WARN] WiFi Failed. Continuing in Offline Mode.");
    lcd.clear(); lcd.print("Offline Mode");
    delay(2000);
  }
}

void setupMQTT() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
}

void reconnectMQTT() {
  if (mqtt.connect("ESP32-Client", MQTT_USER, MQTT_PASS)) {
    Serial.println("MQTT Connected");
    mqtt.subscribe(TOPIC_LAMP_CONTROL);
    mqtt.subscribe(TOPIC_DOOR_CONTROL);
    mqtt.subscribe(TOPIC_CURTAIN_CONTROL);
    mqtt.subscribe(TOPIC_BUZZER_CONTROL);
    mqtt.subscribe(TOPIC_FACE_RECOGNITION);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) message += (char)payload[i];
  
  StaticJsonDocument<256> doc;
  deserializeJson(doc, message);
  String topicStr = String(topic);

  if (topicStr == TOPIC_LAMP_CONTROL) {
    String action = doc["action"];
    if (action == "on") controlLamp(true, doc["mode"]|"manual");
    else if (action == "off") controlLamp(false, doc["mode"]|"manual");
  }
  
  if (topicStr == TOPIC_DOOR_CONTROL) {
    String action = doc["action"];
    if (action == "lock") controlDoor(true, doc["method"]|"remote");
    else if (action == "unlock") controlDoor(false, doc["method"]|"remote");
  }

  if (topicStr == TOPIC_CURTAIN_CONTROL) {
    String action = doc["action"];
    if (action == "open") controlCurtain(true);
    else if (action == "close") controlCurtain(false);
  }

  if (topicStr == TOPIC_BUZZER_CONTROL) {
    String action = doc["action"];
    controlBuzzer(action == "on");
  }

  if (topicStr == TOPIC_FACE_RECOGNITION) {
    String status = doc["status"];
    String userName = doc["user_name"] | "Unknown";
    
    if (status == "success") {
      Serial.printf("[FACE] Recognized: %s\n", userName.c_str());
      controlDoor(false, "face");
      
      lcd.clear();
      lcd.print("Welcome!");
      lcd.setCursor(0, 1);
      lcd.print(userName.substring(0, 16));
      showingMessage = true;
      lcdMessageTimer = millis();
      
      playVoice(VOICE_WELCOME);
      faceRecognitionMode = false;
    } else {
      Serial.println("[FACE] Recognition Failed");
      playVoice(VOICE_FACE_DENIED);
      
      lcd.clear();
      lcd.print("Face Not Found!");
      lcd.setCursor(0, 1);
      lcd.print("Try PIN");
      showingMessage = true;
      lcdMessageTimer = millis();
      
      faceRecognitionMode = false;
    }
  }
}

// ==================== Sensors ====================
void readSensors() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  int mq2Raw = analogRead(MQ2_PIN);
  gasValue = mq2Calibrated ? constrain(map(mq2Raw - mq2Baseline, 0, 1500, 0, 1000), 0, 1000) : map(mq2Raw, 0, 4095, 0, 1000);
  
  lightValue = 1000 - map(analogRead(LDR_PIN), 0, 4095, 0, 1000);
}

void publishSensorData() {
  if (!mqtt.connected()) return;
  StaticJsonDocument<128> doc; char buffer[128];

  doc["temperature"] = temperature; serializeJson(doc, buffer); mqtt.publish(TOPIC_TEMPERATURE, buffer);
  doc.clear(); doc["humidity"] = humidity; serializeJson(doc, buffer); mqtt.publish(TOPIC_HUMIDITY, buffer);
  doc.clear(); doc["ppm"] = gasValue; serializeJson(doc, buffer); mqtt.publish(TOPIC_GAS, buffer);
  doc.clear(); doc["lux"] = lightValue; serializeJson(doc, buffer); mqtt.publish(TOPIC_LIGHT, buffer);
}

// ==================== CONTROLS ====================

void playVoice(int trackNumber) {
  if (dfPlayerReady) {
    dfPlayer.play(trackNumber);
    Serial.printf("[VOICE] Playing Track %d\n", trackNumber);
  }
}

void controlBuzzer(bool state) {
  buzzerState = state;
  if (state) Serial.println("🚨 GAS ALERT -> Buzzer ON");
  else { Serial.println("✅ GAS SAFE -> Buzzer OFF"); digitalWrite(BUZZER_PIN, LOW); }
}

void controlLamp(bool state, String mode) {
  lampState = state; lampMode = mode;
  digitalWrite(RELAY_LAMP_PIN, state ? LOW : HIGH);
  StaticJsonDocument<128> doc; char b[128];
  doc["status"] = state ? "on":"off"; doc["mode"] = mode;
  serializeJson(doc, b); mqtt.publish(TOPIC_LAMP_STATUS, b);
}

void controlDoor(bool lock, String method) {
  doorLocked = lock;
  digitalWrite(RELAY_DOOR_PIN, lock ? HIGH : LOW);

  if (lock) {
    playVoice(VOICE_DOOR_LOCKED);
    doorAutoLockPending = false;
  } else {
    playVoice(VOICE_DOOR_UNLOCKED);
    doorUnlockTimer = millis();
    doorAutoLockPending = true;
  }

  StaticJsonDocument<128> doc; char b[128];
  doc["status"] = lock ? "locked":"unlocked"; doc["method"] = method;
  serializeJson(doc, b); mqtt.publish(TOPIC_DOOR_STATUS, b);
  updateLCD();
}

void controlCurtain(bool open) {
  curtainOpen = open;
  int target = open ? 180 : 0;
  int step = (target > curtainPosition) ? 2 : -2;
  while (curtainPosition != target) {
    curtainPosition += step;
    if ((step > 0 && curtainPosition > target) || (step < 0 && curtainPosition < target)) curtainPosition = target;
    curtainServo.write(curtainPosition); delay(15);
  }
  StaticJsonDocument<128> doc; char b[128];
  doc["status"] = open ? "open":"closed"; doc["position"] = curtainPosition;
  serializeJson(doc, b); mqtt.publish(TOPIC_CURTAIN_STATUS, b);
}

// ==================== KEYPAD LOGIC ====================
void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    // Button A: Trigger Face Recognition
    if (key == 'A') {
      startFaceRecognition();
      return;
    }
    
    // Start PIN entry timer on first digit
    if (key >= '0' && key <= '9') {
      if (enteredPin.length() == 0) {
        pinEntryStartTime = millis();
        pinEntryActive = true;
      }
    }
    
    if (key == '#') {
      if (enteredPin == CORRECT_PIN) {
        Serial.println("PIN CORRECT");
        
        playVoice(VOICE_CORRECT_PIN);
        delay(1500); 

        controlDoor(false, "pin");
        
        delay(1500);
        playVoice(VOICE_WELCOME);

        lcd.clear(); lcd.print("PIN CORRECT!");
        showingMessage = true; lcdMessageTimer = millis();
      } else {
        Serial.println("PIN WRONG");
        playVoice(VOICE_WRONG_PIN); 
        
        lcd.clear(); lcd.print("WRONG PIN!");
        showingMessage = true; lcdMessageTimer = millis();
      }
      enteredPin = "";
    } else if (key == '*') {
      enteredPin = ""; lcd.clear(); lcd.print("Cleared");
      showingMessage = true; lcdMessageTimer = millis();
    } else if (key >= '0' && key <= '9') {
      if (enteredPin.length() < 6) {
        enteredPin += key;
        lcd.clear(); lcd.print("PIN: ");
        for(int i=0; i<enteredPin.length(); i++) lcd.print("*");
        showingMessage = false;
      }
    }
  }
}

void checkDoorAutoLock() {
  if (doorAutoLockPending && !doorLocked) {
    if (millis() - doorUnlockTimer >= DOOR_UNLOCK_DURATION) {
      controlDoor(true, "auto");
      lcd.clear(); lcd.print("AUTO LOCKED");
      showingMessage = true; lcdMessageTimer = millis();
    }
  }
}

void updateLCD() {
  lcd.clear();
  lcd.print("Door: "); lcd.print(doorLocked ? "LOCKED":"OPEN");
  lcd.setCursor(0, 1); lcd.print("Enter PIN");
}

// ==================== PIN TIMEOUT ====================
void checkPinTimeout() {
  if (pinEntryActive && enteredPin.length() > 0) {
    if (millis() - pinEntryStartTime >= PIN_TIMEOUT) {
      Serial.println("[PIN] Timeout - Clearing PIN");
      enteredPin = "";
      pinEntryActive = false;
      
      lcd.clear();
      lcd.print("PIN Timeout!");
      lcd.setCursor(0, 1);
      lcd.print("Cleared");
      showingMessage = true;
      lcdMessageTimer = millis();
    }
  }
}

// ==================== FACE RECOGNITION ====================
void startFaceRecognition() {
  Serial.println("[FACE] Starting face recognition...");
  faceRecognitionMode = true;
  faceRecognitionTimer = millis();
  
  lcd.clear();
  lcd.print("Face Scan...");
  lcd.setCursor(0, 1);
  lcd.print("Look at camera");
  
  // Publish MQTT message to trigger Python face recognition
  if (mqtt.connected()) {
    StaticJsonDocument<128> doc;
    char buffer[128];
    doc["command"] = "scan";
    doc["timestamp"] = millis();
    serializeJson(doc, buffer);
    mqtt.publish("iotcihuy/home/face/trigger", buffer);
    Serial.println("[FACE] Trigger sent via MQTT");
  }
  
  showingMessage = true;
  lcdMessageTimer = millis() + 27000; // Show for 30 seconds
}