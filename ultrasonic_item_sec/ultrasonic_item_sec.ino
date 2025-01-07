#include <IRremote.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>
#include <EEPROM.h>  // Include EEPROM library

// Constants
#define buzzer 8
#define RX_PIN 2 // Fingerprint sensor RX pin
#define TX_PIN 3 // Fingerprint sensor TX pin
#define IR_PIN 4 // IR Receiver pin
#define MAX_ATTEMPTS 3

const int trigPin = 7;
const int echoPin = 6;
const int activatePin = 9;  // System activation button
const int enrollPin = 10;   // Enroll fingerprint
const int deletePin = 11;   // Delete database
const int slotsPin = 12;    // Available slots
const int reexecute = 13;   // Reexecute system

int lastButtonState = 0;   // To track previous state
bool systemActive = false; // To track if the system is active
bool fingerprintVerified = false;  // To track if the fingerprint has been verified

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
  Serial.begin(9600); // For communication with the computer

  // Initialize hardware
  IR.enableIRIn();
  lcd.init();
  lcd.backlight();
  pinMode(trigPin, OUTPUT);  
  pinMode(echoPin, INPUT);  
  pinMode(buzzer, OUTPUT);
  pinMode(activatePin, INPUT);
  pinMode(enrollPin, INPUT);
  pinMode(deletePin, INPUT);
  pinMode(slotsPin, INPUT);
  pinMode(reexecute, INPUT); // Button to re-execute the system

  // Initialize fingerprint sensor
  finger.begin(57600);
  lcd.setCursor(0, 0);
  if (finger.verifyPassword()) {
    lcd.print("Fingerprint Ok");
  } else {
    lcd.print("Sensor Error!");
    while (true);
  }
  delay(2000);
  lcd.clear();
}

void loop() {
  // Check button states
  if (digitalRead(activatePin) == HIGH) {
    toggleSystem();
    delay(200); // Debounce delay
  }

  // Check if the reexecute button is pressed
  if (digitalRead(reexecute) == HIGH) {
    resetSystem(); // Reset the system
    delay(200);  // Debounce delay
  }

  if (systemActive) {
    lcd.setCursor(0, 0);
    lcd.print("Scan your finger");
    if (millis() - timerStart > scanTimeout) {
      systemActive = false;
      timerStart = 0;
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("[ArduScan]                          ");
    } else {
      processFingerprintScan();
    }
  } else {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("[ArduScan]                          ");
    checkAdditionalFunctions();
  }

  // Ultrasonic sensor code for detecting object (only if fingerprint is not verified yet)
  if (!fingerprintVerified) {
    long distance = getDistance();
    if (distance < 20) {  // Object detected
      noTone(buzzer); // Beep loudly
    } else {
      tone(buzzer, 1000); // Stop the beep when no object is detected
    }
  } else {
    noTone(buzzer);  // Once the fingerprint is verified, turn off the buzzer and disable ultrasonic sensor
  }
}

void toggleSystem() {
  systemActive = !systemActive;
  timerStart = millis(); // Start the timer
}

void resetSystem() {
  // Simulate a reset by re-initializing necessary components
  lcd.clear();
  lcd.print("Resetting System...");
  delay(1000);

  // Reset fingerprint verification
  fingerprintVerified = false;

  // Reset ultrasonic sensor pins or any related states
  digitalWrite(trigPin, LOW);
  digitalWrite(echoPin, LOW);  // Set the pins low as a reset step

  // Reinitialize the system (this includes the ultrasonic sensor and fingerprint module)
  setup(); // Re-run setup() to re-initialize everything
}

void processFingerprintScan() {
  int result = getFingerprintID();
  if (result == -1) {
    return;
  }
  lcd.clear();
  lcd.print("Verifying.");
  delay(2000);  // Wait for 2 seconds to show "Verifying."
  lcd.clear();  // Clear the screen before showing "Verified."
  lcd.print("Verified.");
  delay(2000);  // Wait for 2 seconds to show "Verified."
  lcd.clear();  // Clear the screen after showing "Verified."

  // Display only the fingerprint ID
  lcd.setCursor(1, 0);
  lcd.print("--[Welcome]--    ");
  lcd.setCursor(2, 1);
  lcd.print("UniqueID: ");
  lcd.print(result); // Display the fingerprint ID

  tone(buzzer, 1000, 300);
  delay(3000);
  lcd.clear();

  fingerprintVerified = true;  // Set the fingerprint as verified
}

int getFingerprintID() {
  int result = finger.getImage();
  if (result != FINGERPRINT_OK) return -1;
  result = finger.image2Tz();
  if (result != FINGERPRINT_OK) return -1;
  result = finger.fingerSearch();
  if (result != FINGERPRINT_OK) return -1;
  return finger.fingerID;
}

long getDistance() {
  // Trigger the ultrasonic sensor to send a pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure the time it takes for the pulse to return
  long duration = pulseIn(echoPin, HIGH);
  long distance = (duration / 2) / 29.1;  // Calculate distance in cm

  return distance;
}

void enrollFingerprint() {
  lcd.clear();
  lcd.print("Enrolling.             ");
  delay(1000);

  // Find the next available ID
  int id = findAvailableID();
  
  if (id == -1) {
    lcd.clear();
    lcd.print("No Available Slots!");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place Finger");

  // Step 1: Wait for a valid fingerprint image
  while (finger.getImage() != FINGERPRINT_OK) {
    lcd.setCursor(0, 1);
    lcd.print("Try again");
    delay(500);
  }

  // Step 2: Convert image to feature set 1
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Error 1st Img");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.print("Remove Finger");
  delay(2000);

  lcd.clear();
  lcd.print("Same Finger");

  // Step 4: Wait for the same finger again
  while (finger.getImage() != FINGERPRINT_OK) {
    lcd.setCursor(0, 1);
    lcd.print("Try again");
    delay(500);
  }

  // Step 5: Convert image to feature set 2
  if (finger.image2Tz(2) != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Error 2nd Img");
    delay(2000);
    return;
  }

  // Step 6: Create a model
  if (finger.createModel() != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Modeling Error");
    delay(2000);
    return;
  }

  // Step 7: Store the model
  if (finger.storeModel(id) != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Store Error");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.print("Enroll Success!");
  delay(2000);
  lcd.clear();
}

int findAvailableID() {
  int maxSlots = 64; // Maximum slots available on your fingerprint sensor
  for (int id = 1; id <= maxSlots; id++) {
    // Check if the slot is already used
    if (finger.loadModel(id) != FINGERPRINT_OK) {
      return id; // If the ID slot is not used, return it
    }
  }
  return -1; // No available slots
}

void checkAdditionalFunctions() {
  if (digitalRead(enrollPin) == HIGH) {
    enrollFingerprint();
  } else if (digitalRead(deletePin) == HIGH) {
    deleteFingerprintDatabase();
  } else if (digitalRead(slotsPin) == HIGH) {
    checkAvailableSlots();
  }
}

void deleteFingerprintDatabase() {
  lcd.clear();
  lcd.print("DeletingDatabase             ");
  if (finger.emptyDatabase() == FINGERPRINT_OK) {
    lcd.setCursor(0, 1);
    lcd.print("DatabaseDeleted!             ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("DeleteFailed!");
  }
  delay(2000);
  lcd.clear();
}

void checkAvailableSlots() {
  lcd.clear();
  int maxSlots = 64; // Maximum slots available on your fingerprint sensor
  int used = 0;

  // Check each slot to see if it's used
  for (int id = 1; id <= maxSlots; id++) {
    if (finger.loadModel(id) == FINGERPRINT_OK) {
      used++; // If the slot is used, increment the counter
    }
  }

  int available = maxSlots - used; // Available slots are the total slots minus the used ones
  lcd.print("Used: ");
  lcd.print(used);
  lcd.setCursor(0, 1);
  lcd.print("Free: ");
  lcd.print(available); // Accurate slot availability
  delay(3000);
  lcd.clear();
}
