// Include required libraries
#include <MFRC522.h>
//#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <SPI.h>
// Create instances
SoftwareSerial SIM900(3, 4); // SoftwareSerial SIM900(Rx, Tx)
MFRC522 mfrc522(10, 9); // MFRC522 mfrc522(SS_PIN, RST_PIN)
//LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo sg90;
// Initialize Pins for led's, servo and buzzer
// Blue LED is connected to 5V
constexpr uint8_t greenLed = 7;
constexpr uint8_t redLed = 6;
constexpr uint8_t servoPin = 8;
constexpr uint8_t buzzerPin = 5;
char initial_password[4] = {'7', '7', '7', '7'};  // Variable to store initial password
String tagUID = "C9 97 10 5D";  // String to store UID of tag. Change it with your tag's UID
char password[4];   // Variable to store users password
boolean RFIDMode = true; // boolean to change modes
boolean NormalMode = true; // boolean to change modes
char key_pressed = 0; // Variable to store incoming keys
uint8_t i = 0;  // Variable used for counter
// defining how many rows and columns our keypad have
const byte rows = 4;
const byte columns = 4;
// Keypad pin map
char hexaKeys[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// Initializing pins for keypad
byte row_pins[rows] = {A0, A1, A2, A3};
byte column_pins[columns] = {2, 1, 0, 3};
// Create instance for keypad
Keypad keypad_key = Keypad( makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);
void setup() {

    Serial.begin(9600);

  // Arduino Pin configuration
  pinMode(buzzerPin, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  sg90.attach(servoPin);  //Declare pin 8 for servo
  sg90.write(0); // Set initial position at 0 degrees
  //lcd.begin();   // LCD screen
  //lcd.backlight();
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  // Arduino communicates with SIM900 GSM shield at a baud rate of 19200
  // Make sure that corresponds to the baud rate of your module
  SIM900.begin(19200);
  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r");
  delay(100);
  // Set module to send SMS data to serial out upon receipt
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  delay(100);
  //lcd.clear(); // Clear LCD screen
}
void loop() {

  sg90.write(0);

      //Serial.println("looping");

  if (NormalMode == false) {
    // Function to receive message
    receive_message();
  }
  else if (NormalMode == true) {
    // System will first look for mode
    if (RFIDMode == true) {
      // Function to receive message
      receive_message();
      //lcd.setCursor(0, 0);
      Serial.println("   Door Lock");
      //lcd.setCursor(0, 1);
      Serial.println(" Scan Your Tag ");
      // Look for new cards
      if ( ! mfrc522.PICC_IsNewCardPresent()) {
        return;
      }
      // Select one of the cards
      if ( ! mfrc522.PICC_ReadCardSerial()) {
        return;
      }
      //Reading from the card
      String tag = "";
      for (byte j = 0; j < mfrc522.uid.size; j++)
      {
        tag.concat(String(mfrc522.uid.uidByte[j] < 0x10 ? " 0" : " "));
        tag.concat(String(mfrc522.uid.uidByte[j], HEX));
      }
      tag.toUpperCase();
      //Checking the card
      Serial.println("Tag id received = ");
      Serial.println(tag.substring(1));

      if (tag.substring(1) == tagUID)
      {

          sg90.write(0);

        // If UID of tag is matched.
        //lcd.clear();
        Serial.println("Tag Matched");
        digitalWrite(greenLed, HIGH);
        delay(3000);
        digitalWrite(greenLed, LOW);
        //lcd.clear();
        Serial.println("Enter Password:");
        //lcd.setCursor(0, 1);
        RFIDMode = false; // Make RFID mode false
      }
      else
      {
        // If UID of tag is not matched.
        //lcd.clear();
        //lcd.setCursor(0, 0);
        Serial.println("Wrong Tag Shown");
        //lcd.setCursor(0, 1);
        Serial.println("Access Denied");
        digitalWrite(buzzerPin, HIGH);
        digitalWrite(redLed, HIGH);
        send_message("Someone Tried with the wrong tag \nType 'close' to halt the system.");
        delay(3000);
        digitalWrite(buzzerPin, LOW);
        digitalWrite(redLed, LOW);
        //lcd.clear();
      }
    }
    // If RFID mode is false, it will look for keys from keypad
    if (RFIDMode == false) {
      sg90.write(0);

      key_pressed = keypad_key.getKey(); // Storing keys
      if (key_pressed)
      {
              Serial.println(key_pressed);
        password[i++] = key_pressed; // Storing in password variable
        Serial.println("*");
      }
      if (i == 4) // If 4 keys are completed
      {
        delay(200);
        if (!(strncmp(password, initial_password, 4))) // If password is matched
        {
          //lcd.clear();
          Serial.println("Pass Accepted");
          sg90.write(90); // Door Opened
          Serial.println("Take your food. Enjoy your meal.");
          digitalWrite(greenLed, HIGH);
          send_message("Door Opened \nIf it was't you, type 'close' to halt the system.");
          delay(2000);
          Serial.println("Thanks. Door will close in 5 seconds");

          delay(5000);
          digitalWrite(greenLed, LOW);
          
          sg90.write(-90); // Door Closed
          Serial.println("Door Has now closed.");

          //lcd.clear();
          i = 0;
          RFIDMode = true; // Make RFID mode true
        }
        else    // If password is not matched
        {
          //lcd.clear();
          Serial.println("Wrong Password");
          digitalWrite(buzzerPin, HIGH);
          digitalWrite(redLed, HIGH);
          send_message("Someone Tried with the wrong Password \nType 'close' to halt the system.");
          delay(3000);
          digitalWrite(buzzerPin, LOW);
          digitalWrite(redLed, LOW);
          //lcd.clear();
          i = 0;
          RFIDMode = true;  // Make RFID mode true
        }
      }
    }
  }
}
// Receiving the message
void receive_message()
{
  char incoming_char = 0; //Variable to save incoming SMS characters
  String incomingData;   // for storing incoming serial data
  
  if (SIM900.available() > 0)
  {
    incomingData = SIM900.readString(); // Get the incoming data.
    delay(10);
  }
  // if received command is to open the door
  if (incomingData.indexOf("open") >= 0)
  {
    sg90.write(90);
    NormalMode = true;
    send_message("Opened");
    delay(10000);
    sg90.write(0);
  }
  // if received command is to halt the system
  if (incomingData.indexOf("close") >= 0)
  {
    NormalMode = false;
    send_message("Closed");
  }
  incomingData = "";
}
// Function to send the message
void send_message(String message)
{
  SIM900.println("AT+CMGF=1");    //Set the GSM Module in Text Mode
  delay(100);
  SIM900.println("AT+CMGS=\"+XXXXXXXXXXXX\""); // Replace it with your mobile number
  delay(100);
  SIM900.println(message);   // The SMS text you want to send
  delay(100);
  SIM900.println((char)26);  // ASCII code of CTRL+Z
  delay(100);
  SIM900.println();
  delay(1000);
}
