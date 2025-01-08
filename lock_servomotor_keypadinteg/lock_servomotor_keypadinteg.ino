#include <Servo.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Servo setup
Servo myServo;
const int servoPin = 11;

// Keypad setup
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {10, 9, 8, 7}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 5, 4, 3}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Password setup
const String correctPassword = "0211345";
String inputPassword = "";

void setup() {
  // Initialize servo
  myServo.attach(servoPin);
  myServo.write(120); // Lock position

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password:");

  // Begin Serial for debugging
  Serial.begin(9600);
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    if (key == '#') { // Enter key
      if (inputPassword == correctPassword) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Granted");

        // Unlock the servo
        myServo.write(0);
        delay(60000); // Keep unlocked for 5 seconds

        // Relock the servo
        myServo.write(120);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter Password:");
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Denied");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter Password:");
      }
      inputPassword = ""; // Reset the input
    } else if (key == '*') { // Clear key
      inputPassword = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter Password:");
    } else {
      inputPassword += key;
      lcd.setCursor(0, 1);
      lcd.print(inputPassword);
    }

    // Debugging
    Serial.println(inputPassword);
  }
}