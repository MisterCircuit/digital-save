#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// Define the pin connections
#define GREEN_LED 22
#define RED_LED 23
#define BUZZER 10
#define RELAY_PIN 9

// Initialize the fingerprint sensor
#define mySerial Serial1
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Initialize the gsmSerial module
#define gsmSerial Serial2

// Initialize the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad setup
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {8, 7, 6, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 3, 2}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// Variables
int unauthorizedAttempts = 0;
bool lockedOut = false;
const String correctPassword = "2389";
String enteredPassword = "";

// Function to send SMS
void sendSMS(String number, String text)
{
  gsmSerial.println("AT+CMGF=1");    // Set gsmSerial module to text mode
  delay(1000);                 // Wait for a second

  gsmSerial.print("AT+CMGS=\"");     // Command to send SMS
  gsmSerial.print(number);           // Recipient's phone number
  gsmSerial.println("\"");
  delay(1000);                 // Wait for a second

  gsmSerial.println(text);           // Message content
  delay(1000);                 // Wait for a second

  gsmSerial.write(26);               // ASCII code of CTRL+Z (end of message)
  delay(1000);                 // Wait for a second

  Serial.println("Message sent to " + number); // Debugging message
}
void setup() {
  // Initialize components
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  lcd.init();                      // initialize the lcd
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.backlight();

  gsmSerial.begin(9600);
  Serial.begin(9600);
  finger.begin(57600);
  delay(5);

  if (finger.verifyPassword()) {
    lcd.print("Fingerprint found");
  } else {
    lcd.print("Fingerprint not found");
    while (1);
  }
  digitalWrite(RELAY_PIN, LOW); // Ensure safe is locked initially
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place finger:");
}

void loop() {

  //lcd.clear();
  if (!lockedOut) {
    lcd.setCursor(0, 0);
    lcd.print("Place finger:");
    int fingerprintID = getFingerprintID();
    Serial.println(fingerprintID);
    if (fingerprintID == 1) {
      // Authorized access
      lcd.clear();
      lcd.print("Access Granted");
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(BUZZER, HIGH);
      delay (1000);
      digitalWrite(BUZZER, LOW);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RELAY_PIN, HIGH); // Unlock the safe
      delay(10000);
      digitalWrite(RELAY_PIN, LOW); // Lock the safe again
      sendSMS("+2348134369607", "Authorised Access Granted");  // Send to first number
      delay(1000); // Wait for 5 seconds
      sendSMS("+2347061269784", "Authorised Access Granted"); // Send to second number
      delay(1000); // Keep the safe open for 10 seconds   
      lcd.clear();

    } else if (fingerprintID == 0) {
      // Unauthorized attempt
      unauthorizedAttempts++;
      lcd.clear();
      lcd.print("Access Denied");
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);
      delay(2000);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER, LOW);

      if (unauthorizedAttempts >= 3) {
        lcd.clear();
        lcd.print("Locked Out");
        sendSMS("+2348134369607", "Unauthorized attempt to your digital safe detected");  // Send to first number
//        delay(2000); // Wait for 5 seconds
        sendSMS("+2347061269784", "Unauthorized attempt to your digital safe detected"); // Send to second number
        lockedOut = true;
        lcd.clear();
        delay(200);

      }
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Enter Password:");

    char key = keypad.getKey();
    if (key) {
      lcd.clear();
      enteredPassword += key;
      lcd.setCursor(0, 1);
      for (int i = 0; i < enteredPassword.length(); i++) {
        lcd.print('*');
      }

      if (enteredPassword.length() == 4) {
        if (enteredPassword == correctPassword) {
          lcd.clear();
          lcd.print("Access Restored");
          unauthorizedAttempts = 0;
          lockedOut = false;
          enteredPassword = "";
          delay(2000);
          lcd.clear();
        } else {
          lcd.clear();
          lcd.print("Wrong Password");
          enteredPassword = "";
          delay(2000);
        }
      }
    }
  }
}

int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) return -2;
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if ((p == FINGERPRINT_OK && finger.fingerID == 2) || (p == FINGERPRINT_OK && finger.fingerID == 1) || (p == FINGERPRINT_OK && finger.fingerID == 3)|| (p == FINGERPRINT_OK && finger.fingerID == 4)) {
    return 1; // Authorized fingerprint ID
  } else {
    return 0; // Unauthorized fingerprint ID
  }
}
