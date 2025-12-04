/*
 * ==========================================
 * BUZZER TEST - Debugging untuk GPIO 23
 * ==========================================
 * 
 * Wiring:
 *   ESP32 GPIO 23 ──── Buzzer + (Signal)
 *   ESP32 GND     ──── Buzzer - (GND)
 * 
 * Test ini akan:
 * 1. Test buzzer langsung (tanpa MQTT)
 * 2. Test semua GPIO yang mungkin dipakai
 * 3. Cek apakah buzzer Active atau Passive
 */

// ==================== Pin Options ====================
// Coba ganti pin kalau GPIO 23 tidak berfungsi
#define BUZZER_PIN 23   // Default pin

// Alternative pins untuk test:
// #define BUZZER_PIN 2    // Built-in LED (untuk visual test)
// #define BUZZER_PIN 4
// #define BUZZER_PIN 5
// #define BUZZER_PIN 18
// #define BUZZER_PIN 19
// #define BUZZER_PIN 33

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n==========================================");
  Serial.println("       BUZZER DEBUG TEST");
  Serial.println("==========================================");
  Serial.printf("Testing GPIO: %d\n", BUZZER_PIN);
  Serial.println("==========================================\n");
  
  // Setup pin sebagai output
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("Commands via Serial Monitor:");
  Serial.println("  1 = Buzzer ON (constant)");
  Serial.println("  0 = Buzzer OFF");
  Serial.println("  b = Beep 3x");
  Serial.println("  p = PWM test (passive buzzer)");
  Serial.println("  t = Toggle test");
  Serial.println("  a = All pins test");
  Serial.println("==========================================\n");
}

void loop() {
  // Auto test setiap 5 detik
  static unsigned long lastTest = 0;
  static int testPhase = 0;
  
  if (millis() - lastTest >= 5000) {
    lastTest = millis();
    
    switch (testPhase) {
      case 0:
        Serial.println("\n[AUTO TEST 1] Buzzer ON for 1 second...");
        digitalWrite(BUZZER_PIN, HIGH);
        delay(1000);
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("Buzzer OFF - Did you hear it?");
        break;
        
      case 1:
        Serial.println("\n[AUTO TEST 2] Beeping 5x...");
        beepBuzzer(5);
        break;
        
      case 2:
        Serial.println("\n[AUTO TEST 3] PWM tone test (passive buzzer)...");
        pwmTest();
        break;
        
      case 3:
        Serial.println("\n[AUTO TEST 4] Fast toggle test...");
        toggleTest();
        break;
    }
    
    testPhase = (testPhase + 1) % 4;
  }
  
  // Manual control via Serial
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case '1':
        Serial.println(">>> Buzzer ON (constant)");
        digitalWrite(BUZZER_PIN, HIGH);
        break;
        
      case '0':
        Serial.println(">>> Buzzer OFF");
        digitalWrite(BUZZER_PIN, LOW);
        break;
        
      case 'b':
        Serial.println(">>> Beeping 3x...");
        beepBuzzer(3);
        break;
        
      case 'p':
        Serial.println(">>> PWM test...");
        pwmTest();
        break;
        
      case 't':
        Serial.println(">>> Toggle test...");
        toggleTest();
        break;
        
      case 'a':
        Serial.println(">>> Testing ALL common GPIO pins...");
        testAllPins();
        break;
    }
  }
}

// ==================== Test Functions ====================

void beepBuzzer(int count) {
  Serial.printf("Beeping %d times on GPIO %d\n", count, BUZZER_PIN);
  
  for (int i = 0; i < count; i++) {
    Serial.printf("  Beep %d: HIGH\n", i + 1);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    
    Serial.printf("  Beep %d: LOW\n", i + 1);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
  
  Serial.println("Beeping done!");
}

void pwmTest() {
  // Test untuk PASSIVE buzzer (butuh PWM/frequency)
  Serial.println("PWM Test - untuk passive buzzer");
  
  // Tone frequencies
  int frequencies[] = {262, 294, 330, 349, 392, 440, 494, 523};  // C4 to C5
  
  for (int i = 0; i < 8; i++) {
    Serial.printf("  Freq: %d Hz\n", frequencies[i]);
    
    // Generate tone manually
    int period = 1000000 / frequencies[i];  // microseconds
    int halfPeriod = period / 2;
    
    unsigned long start = millis();
    while (millis() - start < 200) {  // 200ms per note
      digitalWrite(BUZZER_PIN, HIGH);
      delayMicroseconds(halfPeriod);
      digitalWrite(BUZZER_PIN, LOW);
      delayMicroseconds(halfPeriod);
    }
    
    delay(50);
  }
  
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("PWM test done!");
}

void toggleTest() {
  Serial.println("Fast toggle test (500ms interval)");
  
  for (int i = 0; i < 10; i++) {
    bool state = (i % 2 == 0);
    digitalWrite(BUZZER_PIN, state);
    Serial.printf("  Toggle %d: %s\n", i + 1, state ? "HIGH" : "LOW");
    delay(500);
  }
  
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Toggle test done!");
}

void testAllPins() {
  // Test beberapa GPIO yang umum dipakai
  int testPins[] = {2, 4, 5, 18, 19, 23, 25, 26, 27, 32, 33};
  int numPins = sizeof(testPins) / sizeof(testPins[0]);
  
  Serial.println("\n=== TESTING ALL GPIO PINS ===");
  Serial.println("Connect buzzer to each pin and listen:\n");
  
  for (int i = 0; i < numPins; i++) {
    int pin = testPins[i];
    
    Serial.printf("Testing GPIO %d... ", pin);
    
    pinMode(pin, OUTPUT);
    
    // 3 quick beeps
    for (int j = 0; j < 3; j++) {
      digitalWrite(pin, HIGH);
      delay(100);
      digitalWrite(pin, LOW);
      delay(100);
    }
    
    Serial.println("DONE");
    delay(500);
  }
  
  // Reset original pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("\n=== ALL PINS TESTED ===");
  Serial.println("Which GPIO worked? Update BUZZER_PIN accordingly.\n");
}
