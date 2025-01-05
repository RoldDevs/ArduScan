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

const int buttonPin = 9;
int buttonState = 0;
int lastButtonState = 0; // To track previous state
bool systemActive = false; // To track if the system is active

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fingerprint sensor
SoftwareSerial mySerial(RX_PIN, TX_PIN); 
Adafruit_Fingerprint finger(&mySerial);
IRrecv IR(IR_PIN); // IR Receiver
decode_results results;

unsigned long timerStart = 0;  // Timer start time
const unsigned long scanTimeout = 10000; // Timeout period (10 seconds)

void setup() {
  Serial.begin(9600); // For debugging

  // Remote setup
  IR.enableIRIn();

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing.");
  
  // Initialize buzzer
  pinMode(buzzer, OUTPUT);
  noTone(buzzer);

  // Initialize button pin
  pinMode(buttonPin, INPUT); // Configure the button pin
  
  // Initialize fingerprint sensor
  finger.begin(57600); // Default baud rate
  if (finger.verifyPassword()) {
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint Ok");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Sensor Error!");
    while (true); // Halt execution if fingerprint sensor fails
  }

  delay(2000);
  lcd.clear();
}

void loop() {
  buttonState = digitalRead(buttonPin);
  
  // If button state changes (pressed or released), toggle systemActive
  if (buttonState == HIGH && lastButtonState == LOW) {
    systemActive = !systemActive; // Toggle system active state
    delay(200);  // Debounce delay
  }
  lastButtonState = buttonState;

  if (systemActive) {
    // If system is active, allow fingerprint scan
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan your finger");

    if (timerStart == 0) {
      // Start the timer when system is activated
      timerStart = millis();
    }

    // If 10 seconds have passed, go back to "Choose option"
    if (millis() - timerStart > scanTimeout) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("[ArduScan]              ");
      timerStart = 0;  // Reset the timer
      systemActive = false;  // Deactivate the system after timeout
    } else {
      processFingerprintScan();
    }
  } else {
    // If system is inactive, show "Choose option"
    lcd.clear();   // Ensure the screen is cleared
    lcd.setCursor(3, 0);
    lcd.print("[ArduScan]                ");
    lcd.backlight();  // Ensure backlight is on
    timerStart = 0;  // Reset the timer
  }
}

void processFingerprintScan() {
  int result = getFingerprintID();
  if (result == -1) {
    // No fingerprint detected, remain in standby
    return;
  }

  int attempts = 0;
  while (attempts < MAX_ATTEMPTS) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Processing.");

    if (result != -1) {
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
}

int getFingerprintID() {
  int result = finger.getImage();
  if (result != FINGERPRINT_OK) {
    return -1; // No fingerprint detected
  }

  result = finger.image2Tz();
  if (result != FINGERPRINT_OK) {
    return -1; // Failed to process fingerprint
  }

  result = finger.fingerSearch();
  if (result != FINGERPRINT_OK) {
    return -1; // No match found
  }

  // Fingerprint recognized
  Serial.print("Fingerprint matched! ID: ");
  Serial.println(finger.fingerID);
  return finger.fingerID;
}
