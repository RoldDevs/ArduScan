//MainProgrammer: Harold Edsel F. Cabaluna, Vergel O. Macayan
//HardWare: Allan Hover Bonifacio
//CodingMembers: Gabryelle T. Hermosa, Jonas H. Alberca

/**
 BSCS - 4A [Valid Identification Scanner with Arduino]
 Project Name: ArduScan

  Libraries Used:
   - MFRC522 {RFID CARD}
   - Adafruit Fingerprint Sensor {FingerPrint Scanner}
   - LiquidCrystal_I2C.h {Display For Output}
   - ABM (URL's) for ESP32 CAM {https://dl.espressif.com/dl/package_esp32_index.json} || Arduino ESP32 Board
   - IR Remote {IRremote} for infrared Remote Control
*/

#include <IRremote.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>

// Constants
#define buzzer 8
#define RX_PIN 2 // Fingerprint sensor RX pin
#define TX_PIN 3 // Fingerprint sensor TX pin (Changed to avoid conflict with IR)
#define IR_PIN 4 // IR Receiver pin
#define MAX_ATTEMPTS 3
#define OK_BUTTON_CODE 0xFFA25D // Replace with your remote's "OK" button code

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fingerprint sensor
SoftwareSerial mySerial(RX_PIN, TX_PIN); 
Adafruit_Fingerprint finger(&mySerial);
IRrecv IR(IR_PIN); // IR Receiver
decode_results results;

void setup() {
  Serial.begin(9600); // For debugging

  // Remote setup
  IR.enableIRIn();
  Serial.println("IR Receiver Initialized");

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  Serial.println("Initializing LCD and Fingerprint Sensor...");
  
  // Initialize buzzer
  pinMode(buzzer, OUTPUT);
  noTone(buzzer);

  // Initialize fingerprint sensor
  finger.begin(57600); // Default baud rate
  if (finger.verifyPassword()) {
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint OK!");
    Serial.println("Fingerprint sensor initialized successfully.");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Sensor Error!");
    Serial.println("Error: Fingerprint sensor not detected.");
    while (true); // Halt execution if fingerprint sensor fails
  }

  delay(2000);
  lcd.clear();
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("Place your finger");
  int attempts = 0;

  while (attempts < MAX_ATTEMPTS) {
    int result = getFingerprintID();
    if (result == -1) {
      // Fingerprint not recognized
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Please try again");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Place your finger");
      attempts++;
    } else {
      // Fingerprint recognized
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access Granted!");
      tone(buzzer, 1000, 300); // Short beep for success
      delay(3000);
      lcd.clear();
      return;
    }
  }

  // After 3 failed attempts
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Failed Attempts!");

  unsigned long startTime = millis();
  tone(buzzer, 4000); // Continuous beep

  // Check for remote signal while beeping
  while (millis() - startTime < 5000) { // 5 seconds duration
    if (IR.decode(&results)) {
      if (results.value == OK_BUTTON_CODE) {
        noTone(buzzer); // Stop the buzzer
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Buzzer Stopped");
        delay(2000);
        lcd.clear();
        return;
      }
      IR.resume(); // Receive the next IR signal
    }
  }

  noTone(buzzer); // Stop buzzer after 5 seconds
  lcd.clear();
}

int getFingerprintID() {
  int result = finger.getImage();
  if (result != FINGERPRINT_OK) {
    Serial.println("Failed to get image.");
    return -1;
  }

  result = finger.image2Tz();
  if (result != FINGERPRINT_OK) {
    Serial.println("Failed to convert image.");
    return -1;
  }

  result = finger.fingerSearch();
  if (result != FINGERPRINT_OK) {
    Serial.println("No match found.");
    return -1;
  }

  Serial.print("Fingerprint matched! ID: ");
  Serial.println(finger.fingerID);
  return finger.fingerID;
}
