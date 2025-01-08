#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define SS_PIN 10
#define RST_PIN 9
#define LED_G 3 //define green LED pin
#define LED_R 2 //define red LED
#define LED_B 4 //define blue LED
#define LED_Y 5 //define yellow LED
#define TRIG_PIN 7
#define ECHO_PIN 6
Servo myServo; //define servo name
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

enum State {
  STATE_FUNCTIONAL,
  STATE_CAR_DETECTED,
  STATE_WAITING_FOR_UID,
  STATE_UID_CONFIRMED,
  STATE_BARRIER_OPENING,
  STATE_WAITING_FOR_OTHER_CAR,
  STATE_FORCED_OPEN,
  STATE_CLOSING
};

State currentState = STATE_FUNCTIONAL;
int cm;
long lastDetectionTime = 0;
const int set_time = 2000;

// List of authorized UIDs
byte authorizedUIDs[][4] = {
  {0X03, 0X33, 0X32, 0X14},
  {0XA3, 0X21, 0X2C, 0X14}
};
const int numAuthorizedUIDs = 2;

void setup() {
  Serial.begin(9600);   // Initiate a serial communication
  SPI.begin();      // Initiate SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  myServo.attach(8); //servo pin
  myServo.write(0); //servo start position
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("Put your card to the reader...");
  Serial.println();
}

bool isAuthorized(byte* uid, byte uidSize) {
  for (int i = 0; i < numAuthorizedUIDs; i++) {
    bool match = true;
    for (byte j = 0; j < uidSize; j++) {
      if (authorizedUIDs[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

void loop() {
  // Measure the distance
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  cm = duration / 29 / 2;

  Serial.print("cm:"); Serial.println(cm);

  switch (currentState) {
    case STATE_FUNCTIONAL:
      allLedsOff();
      if (cm < 10) {
        Serial.println("Car detected");
        currentState = STATE_CAR_DETECTED;
      }
      break;

    case STATE_CAR_DETECTED:
      Serial.println("State: CAR_DETECTED");
      digitalWrite(LED_B, HIGH);
      currentState = STATE_WAITING_FOR_UID;
      break;

    case STATE_WAITING_FOR_UID:
      Serial.println("State: WAITING_FOR_UID");
      digitalWrite(LED_B, LOW);
      digitalWrite(LED_Y, HIGH);
      if (mfrc522.PICC_IsNewCardPresent()) {
        Serial.println("Card present");
        if (mfrc522.PICC_ReadCardSerial()) {
          Serial.print("Read UID: ");
          for (byte i = 0; i < mfrc522.uid.size; i++) {
            Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            Serial.print(mfrc522.uid.uidByte[i], HEX);
          }
          Serial.println();
          if (isAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size)) {
            Serial.println("UID autorizat");
            currentState = STATE_UID_CONFIRMED;
          } else {
            Serial.println("UID neautorizat");
            currentState = STATE_FUNCTIONAL; // Sau alta stare pentru UID neautorizat
          }
        } else {
          Serial.println("Failed to read card serial");
        }
      }
      break;

    case STATE_UID_CONFIRMED:
      Serial.println("State: UID_CONFIRMED");
      digitalWrite(LED_B, HIGH);
      digitalWrite(LED_Y, HIGH);
      delay(2000);
      currentState = STATE_BARRIER_OPENING;
      break;

    case STATE_BARRIER_OPENING:
      Serial.println("State: BARRIER_OPENING");
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_B, LOW);
      digitalWrite(LED_Y, LOW);
      myServo.write(90);
      currentState = STATE_WAITING_FOR_OTHER_CAR;
      lastDetectionTime = millis();
      break;

    case STATE_WAITING_FOR_OTHER_CAR:
      Serial.println("State: WAITING_FOR_OTHER_CAR");
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_Y, HIGH);
      if (cm < 10) {
        lastDetectionTime = millis();
      } else if (millis() - lastDetectionTime > set_time) {
        currentState = STATE_CLOSING;
      }
      break;

    case STATE_FORCED_OPEN:
      Serial.println("State: FORCED_OPEN");
      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, HIGH);
      break;

    case STATE_CLOSING:
      Serial.println("State: CLOSING");
      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      digitalWrite(LED_Y, LOW);
      myServo.write(0);
      currentState = STATE_FUNCTIONAL;
      break;
  }

  delay(400);
}

void allLedsOff() {
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_B, LOW);
  digitalWrite(LED_Y, LOW);
}