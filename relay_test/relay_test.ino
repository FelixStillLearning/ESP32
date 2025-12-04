/*
  RELAY & SOLENOID TEST
  ======================
  Test relay IN1 di GPIO 26 untuk solenoid door lock
  
  Wiring:
  - ESP32 GPIO 26 → Relay IN1
  - ESP32 5V      → Relay VCC
  - ESP32 GND     → Relay GND
  - PSU 12V+      → Relay COM1
  - Relay NO1     → Solenoid (+)
  - PSU 12V GND   → Solenoid (-)
  - PSU 12V GND   → ESP32 GND (COMMON GROUND!)
*/

#define RELAY_PIN 26

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n========================================");
  Serial.println("RELAY & SOLENOID TEST");
  Serial.println("========================================");
  Serial.println("Relay Pin: GPIO 26");
  Serial.println("========================================\n");
  
  // Set pin mode
  pinMode(RELAY_PIN, OUTPUT);
  
  // Start with relay OFF (HIGH for active-low relay)
  digitalWrite(RELAY_PIN, HIGH);
  
  Serial.println("[TEST 1] Initial state: RELAY OFF (GPIO HIGH)");
  Serial.println("         Relay LED should be OFF");
  Serial.println("         Solenoid should be OFF (locked)\n");
  
  delay(3000);
  
  // Test relay ON
  Serial.println("[TEST 2] Turning RELAY ON (GPIO LOW)...");
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("         Relay should CLICK!");
  Serial.println("         Relay LED should be ON");
  Serial.println("         Solenoid should activate (unlock)\n");
  
  delay(3000);
  
  // Test relay OFF
  Serial.println("[TEST 3] Turning RELAY OFF (GPIO HIGH)...");
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("         Relay should CLICK again!");
  Serial.println("         Relay LED should be OFF");
  Serial.println("         Solenoid should deactivate (lock)\n");
  
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("INTERACTIVE TEST MODE");
  Serial.println("========================================");
  Serial.println("Commands (type in Serial Monitor):");
  Serial.println("  '1' = Relay ON  (GPIO LOW)  - Unlock");
  Serial.println("  '0' = Relay OFF (GPIO HIGH) - Lock");
  Serial.println("  't' = Toggle relay");
  Serial.println("  'p' = Pulse test (ON 1s, OFF 1s) x3");
  Serial.println("  'r' = Read GPIO state");
  Serial.println("========================================\n");
}

bool relayState = false;

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case '1':
        digitalWrite(RELAY_PIN, LOW);  // Active LOW = Relay ON
        relayState = true;
        Serial.println("\n✓ RELAY ON (GPIO 26 = LOW)");
        Serial.println("  → Solenoid should be ENERGIZED (unlocked)");
        Serial.println("  → Relay LED should be ON");
        Serial.printf("  → GPIO 26 read: %d\n\n", digitalRead(RELAY_PIN));
        break;
        
      case '0':
        digitalWrite(RELAY_PIN, HIGH);  // Active LOW = Relay OFF
        relayState = false;
        Serial.println("\n✓ RELAY OFF (GPIO 26 = HIGH)");
        Serial.println("  → Solenoid should be DE-ENERGIZED (locked)");
        Serial.println("  → Relay LED should be OFF");
        Serial.printf("  → GPIO 26 read: %d\n\n", digitalRead(RELAY_PIN));
        break;
        
      case 't':
        relayState = !relayState;
        digitalWrite(RELAY_PIN, relayState ? LOW : HIGH);
        Serial.printf("\n✓ TOGGLED → Relay is now %s\n", relayState ? "ON" : "OFF");
        Serial.printf("  → GPIO 26 = %s\n\n", relayState ? "LOW" : "HIGH");
        break;
        
      case 'p':
        Serial.println("\n[PULSE TEST] Starting 3x pulse...");
        for (int i = 1; i <= 3; i++) {
          Serial.printf("  Pulse %d: ON...", i);
          digitalWrite(RELAY_PIN, LOW);
          delay(1000);
          Serial.println(" OFF");
          digitalWrite(RELAY_PIN, HIGH);
          delay(1000);
        }
        Serial.println("[PULSE TEST] Complete!\n");
        break;
        
      case 'r':
        Serial.println("\n[GPIO READ]");
        Serial.printf("  GPIO 26 state: %d\n", digitalRead(RELAY_PIN));
        Serial.printf("  Relay should be: %s\n\n", digitalRead(RELAY_PIN) ? "OFF" : "ON");
        break;
        
      default:
        // Ignore other characters (like newline)
        break;
    }
  }
  
  delay(10);
}
