/*
  KEYPAD MAPPING TEST
  ====================
  Upload ini dulu untuk cari tahu mapping keypad yang benar.
  Tekan setiap tombol dan catat output di Serial Monitor.
*/

#include <Keypad.h>

const byte ROWS = 4;
const byte COLS = 4;

// Mapping STANDAR - kita akan test ini dulu
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Pin assignment yang kamu pakai
byte rowPins[ROWS] = {19, 5, 15, 12};  // Row 1-4
byte colPins[COLS] = {18, 4, 27, 14};   // Col 1-4

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n========================================");
  Serial.println("KEYPAD MAPPING TEST");
  Serial.println("========================================");
  Serial.println("\nTekan setiap tombol dan catat hasilnya:");
  Serial.println("Contoh: Tekan '1' -> lihat output apa");
  Serial.println("\nKeypad Layout yang diharapkan:");
  Serial.println("  [1] [2] [3] [A]");
  Serial.println("  [4] [5] [6] [B]");
  Serial.println("  [7] [8] [9] [C]");
  Serial.println("  [*] [0] [#] [D]");
  Serial.println("\n========================================");
  Serial.println("Mulai tekan tombol...\n");
}

void loop() {
  char key = keypad.getKey();
  
  if (key) {
    Serial.print("Tombol ditekan -> Output: '");
    Serial.print(key);
    Serial.print("' (ASCII: ");
    Serial.print((int)key);
    Serial.println(")");
    
    // Identify position
    for (int r = 0; r < ROWS; r++) {
      for (int c = 0; c < COLS; c++) {
        if (keys[r][c] == key) {
          Serial.print("  Position: Row ");
          Serial.print(r);
          Serial.print(", Col ");
          Serial.println(c);
        }
      }
    }
    Serial.println();
  }
}
