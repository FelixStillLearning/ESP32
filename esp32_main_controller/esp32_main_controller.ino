/*
 * ESP32 Smart Home Main Controller
 * FIXED VERSION - Tested & Working
 * 
 * Hardware Test Results:
 * ✅ LCD I2C: 0x27
 * ✅ DHT22: 27.8°C, 81%
 * ✅ LDR: 4095 (working)
 * ✅ MQ-2: 1518 (working)
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ArduinoJson.h>

// ==================== WiFi Configuration ====================
const char* WIFI_SSID = "FELIXTHE3RD";
const char* WIFI_PASS = "superadmin";

// ==================== MQTT Configuration ====================
const char* MQTT_SERVER = "10.224.227.57";  // IP Mosquitto broker
const int MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASS = "";

// MQTT Topics - Publish (Sensor Data)
const char* TOPIC_TEMPERATURE = "home/temperature";
const char* TOPIC_HUMIDITY = "home/humidity";
const char* TOPIC_GAS = "home/gas";
const char* TOPIC_LIGHT = "home/light";

// MQTT Topics - Subscribe (Control Commands)
const char* TOPIC_LAMP_CONTROL = "home/lamp/control";
const char* TOPIC_DOOR_CONTROL = "home/door/control";

// MQTT Topics - Status (Feedback to server)
const char* TOPIC_LAMP_STATUS = "home/lamp/status";
const char* TOPIC_DOOR_STATUS = "home/door/status";

// ==================== Pin Definitions ====================
// DHT Sensor - FIXED: DHT22 not DHT11!
#define DHT_PIN 13
#define DHT_TYPE DHT22

// Analog Sensors - GPIO12 kadang bermasalah, pindah ke GPIO lain
#define LDR_PIN 32   // Pindah dari GPIO 12 ke GPIO 32 (ADC1_CH4)
#define MQ2_PIN 33   // Pindah dari GPIO 14 ke GPIO 33 (ADC1_CH5)

// Relay
#define RELAY_PIN 27

// LCD I2C - FIXED: Address 0x27
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Keypad Configuration
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {15, 2, 0, 4};
byte colPins[COLS] = {16, 17, 5, 18};

// ==================== Objects ====================
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ==================== Variables ====================
float temperature = 0;
float humidity = 0;
int gasValue = 0;
int lightValue = 0;

bool lampState = false;
String lampMode = "manual";
bool doorLocked = true;

unsigned long lastSensorRead = 0;
unsigned long lastMQTTPublish = 0;
const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long MQTT_INTERVAL = 5000;

String enteredPin = "";
const String CORRECT_PIN = "1234";

int displayMode = 0;
unsigned long lastDisplayChange = 0;
const unsigned long DISPLAY_INTERVAL = 3000;

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

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("ESP32 Smart Home Controller Starting...");
  Serial.println("========================================");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  dht.begin();
  delay(2000);  // DHT22 needs time to stabilize

  setupWiFi();
  setupMQTT();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);

  Serial.println("✅ System initialized successfully!");
}

// ==================== Main Loop ====================
void loop() {
  if (!mqtt.connected()) {
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

  if (millis() - lastDisplayChange >= DISPLAY_INTERVAL) {
    displayMode = (displayMode + 1) % 4;
    updateLCD();
    lastDisplayChange = millis();
  }
}

// ==================== WiFi Setup ====================
void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(1500);
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
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
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT Connecting");

    String clientId = "ESP32-SmartHome-" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("✅ Connected!");

      mqtt.subscribe(TOPIC_LAMP_CONTROL);
      mqtt.subscribe(TOPIC_DOOR_CONTROL);

      Serial.println("Subscribed to:");
      Serial.println("  - " + String(TOPIC_LAMP_CONTROL));
      Serial.println("  - " + String(TOPIC_DOOR_CONTROL));

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MQTT Connected!");
      delay(500);
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Retrying in 5 seconds...");

      lcd.setCursor(0, 1);
      lcd.print("Retry in 5s...");
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

  Serial.println("📥 MQTT Message:");
  Serial.println("  Topic: " + String(topic));
  Serial.println("  Payload: " + message);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.println("❌ JSON parsing failed!");
    return;
  }

  if (String(topic) == TOPIC_LAMP_CONTROL) {
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

  if (String(topic) == TOPIC_DOOR_CONTROL) {
    String action = doc["action"] | "";
    String method = doc["method"] | "remote";

    if (action == "lock") {
      controlDoor(true, method);
    } else if (action == "unlock") {
      controlDoor(false, method);
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

  // Read MQ-2 Gas sensor (raw ADC value: 0-4095)
  int mq2Raw = analogRead(MQ2_PIN);
  // Convert to PPM - use constrain to ensure valid range
  gasValue = constrain(map(mq2Raw, 0, 4095, 0, 1000), 0, 1000);
  
  // Alternative: just use raw value scaled down
  // gasValue = mq2Raw / 4;  // Scale 0-4095 to ~0-1000

  // Read LDR (raw ADC value: 0-4095)
  int ldrRaw = analogRead(LDR_PIN);
  // LDR inverse: higher raw = brighter light
  // Scale to 0-1000 lux range
  lightValue = constrain(map(ldrRaw, 0, 4095, 0, 1000), 0, 1000);
  
  // Alternative: just use raw value scaled down
  // lightValue = ldrRaw / 4;  // Scale 0-4095 to ~0-1000

  Serial.println("📊 Sensor Readings:");
  Serial.printf("  Temperature: %.1f°C\n", temperature);
  Serial.printf("  Humidity: %.1f%%\n", humidity);
  Serial.printf("  Gas (MQ2 raw): %d, PPM: %d\n", mq2Raw, gasValue);
  Serial.printf("  Light (LDR raw): %d, Lux: %d\n", ldrRaw, lightValue);
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
  Serial.println("📤 Published temperature");

  // Publish Humidity
  doc.clear();
  doc["humidity"] = humidity;
  doc["unit"] = "percent";
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_HUMIDITY, buffer);
  Serial.println("📤 Published humidity");

  // Publish Gas
  doc.clear();
  doc["ppm"] = gasValue;
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_GAS, buffer);
  Serial.println("📤 Published gas");

  // Publish Light
  doc.clear();
  doc["lux"] = lightValue;
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_LIGHT, buffer);
  Serial.println("📤 Published light");
}

// ==================== Device Control ====================
void controlLamp(bool state, String mode) {
  lampState = state;
  lampMode = mode;

  digitalWrite(RELAY_PIN, state ? LOW : HIGH);

  Serial.printf("💡 Lamp: %s (%s mode)\n", state ? "ON" : "OFF", mode.c_str());

  StaticJsonDocument<128> doc;
  char buffer[128];
  doc["status"] = state ? "on" : "off";
  doc["mode"] = mode;
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_LAMP_STATUS, buffer);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Lamp: ");
  lcd.print(state ? "ON" : "OFF");
  lcd.setCursor(0, 1);
  lcd.print("Mode: ");
  lcd.print(mode);
  lastDisplayChange = millis();
}

void controlDoor(bool lock, String method) {
  doorLocked = lock;

  Serial.printf("🚪 Door: %s (%s)\n", lock ? "LOCKED" : "UNLOCKED", method.c_str());

  StaticJsonDocument<128> doc;
  char buffer[128];
  doc["status"] = lock ? "locked" : "unlocked";
  doc["method"] = method;
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_DOOR_STATUS, buffer);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Door: ");
  lcd.print(lock ? "LOCKED" : "UNLOCKED");
  lcd.setCursor(0, 1);
  lcd.print("Via: ");
  lcd.print(method);
  lastDisplayChange = millis();
}

// ==================== Keypad Handling ====================
void handleKeypad() {
  char key = keypad.getKey();

  if (key) {
    Serial.printf("🔢 Key pressed: %c\n", key);

    if (key == '#') {
      if (enteredPin == CORRECT_PIN) {
        Serial.println("✅ PIN Correct!");
        controlDoor(!doorLocked, "keypad");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("PIN Correct!");
        lcd.setCursor(0, 1);
        lcd.print(doorLocked ? "Locked" : "Unlocked");
      } else {
        Serial.println("❌ PIN Incorrect!");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wrong PIN!");
        lcd.setCursor(0, 1);
        lcd.print("Try again...");
      }
      enteredPin = "";
      lastDisplayChange = millis();
    } else if (key == '*') {
      enteredPin = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PIN Cleared");
      lastDisplayChange = millis();
    } else if (key == 'A') {
      controlLamp(!lampState, "manual");
    } else if (key == 'B') {
      displayMode = (displayMode + 1) % 4;
      updateLCD();
      lastDisplayChange = millis();
    } else {
      if (enteredPin.length() < 6) {
        enteredPin += key;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter PIN:");
        lcd.setCursor(0, 1);
        for (int i = 0; i < enteredPin.length(); i++) {
          lcd.print("*");
        }
      }
    }
  }
}

// ==================== LCD Update ====================
void updateLCD() {
  lcd.clear();

  switch (displayMode) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("T:");
      lcd.print(temperature, 1);
      lcd.print("C H:");
      lcd.print(humidity, 0);
      lcd.print("%");
      lcd.setCursor(0, 1);
      lcd.print("G:");
      lcd.print(gasValue);
      lcd.print(" L:");
      lcd.print(lightValue);
      break;

    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Lamp: ");
      lcd.print(lampState ? "ON" : "OFF");
      lcd.setCursor(0, 1);
      lcd.print("Door: ");
      lcd.print(doorLocked ? "LOCK" : "OPEN");
      break;

    case 2:
      lcd.setCursor(0, 0);
      lcd.print("WiFi: ");
      lcd.print(WiFi.status() == WL_CONNECTED ? "OK" : "ERR");
      lcd.setCursor(0, 1);
      lcd.print("MQTT: ");
      lcd.print(mqtt.connected() ? "OK" : "ERR");
      break;

    case 3:
      lcd.setCursor(0, 0);
      lcd.print(WiFi.localIP());
      lcd.setCursor(0, 1);
      lcd.print("Press B:Next");
      break;
  }
}