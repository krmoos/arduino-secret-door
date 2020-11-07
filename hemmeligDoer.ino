

//Speaker
const int soundPin = 3;
const int volume = 2; //normal=3

//Motor
const int pulsePin      = 7;
const int directionPin  = 6; //motor direction - LOW = go up
const int enablePin     = 2; //momentum - HIGH = motor keep holding wire 
const long motorStepDistance = 5000;
bool goUp = true;

//Globals for LEDring
#include <FastLED.h>
#define NUM_LEDS 24 // How many leds are in the strip?
#define DATA_PIN 5 // Data pin that led data will be written out over
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS]; // This is an array of leds.  One item for each led in your strip.
#define BRIGHTNESS          40
#define FRAMES_PER_SECOND  120

CRGBPalette16 currentPalette;
TBlendType    currentBlending;
extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;



// Globals for Ottos sensor 
const int sensorPin = 4; //Touch sensor - Capacity sensor
const int numPresses = 5; //Number of presses - code
const int thresh = 200;
int pressIndex = 0;
int len;
bool haveResetCount = false;
int pressTimes[numPresses];
int correctPressTimes[] = {200,200,200,500,200};
CRGB pressColor = CRGB::Blue;

/* Globals for Askes sensor */
typedef struct {
  int  num;
  boolean access;
  String macAdress;
  CRGB color;//String color;
  String desc;
  String type;
} RFIDcardType;

RFIDcardType RFIDcards[] = { 
//num,access,macAdress    ,colorLED ,Sign                 ,type  
  {1 ,true  ,"57 51 87 4B",CRGB::Yellow ,"Rød-prikket"        ,"nøglebrik"},
  {2 ,true  ,"97 A1 E5 33",CRGB::Green  ,"Sort"               ,"nøglebrik"},
  {3 ,true  ,"FD 34 D0 2B",CRGB::Gold  ,"BD-kort"            ,"Kort"},
  {4 ,true  ,"87 0E 21 65",CRGB::Yellow   ,"Hvidt nr. 1"        ,"Kort"},
  {5 ,true  ,"72 13 DF 20",CRGB::Blue  ,"Hvidt nr. 2"        ,"Kort"},
  {6 ,true  ,"30 F7 26 75",CRGB::Red  ,"Hvidt nr. 3"        ,"Kort"}
};

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

void setup() {
  Serial.begin(9600);
  char buffer[30];
  
  pinMode(sensorPin, INPUT);
  Serial.println("[PRESS] klar...");
  
  pinMode(soundPin, OUTPUT);
  sprintf(buffer, "[SOUND] klar... volume=%d ", volume);
  Serial.println(buffer);
  
  pinMode(pulsePin, OUTPUT);
  pinMode(directionPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  digitalWrite(directionPin,HIGH);
  Serial.println("[MOTOR] klar...");
  
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  len = sizeof(RFIDcards) / sizeof(RFIDcards[0]); // how many elements in array
  sprintf(buffer, "[RFID] klar... kort i databasen=%d", len);
  Serial.println(buffer);
  
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  Serial.println("[LED] klar...");
  Serial.println();

}

void loop() {
  long loopStartTime = millis();
  
  while(!digitalRead(sensorPin)) { //primarialy in this while (not pushing)
    checkRFID();
    // Resets after 10 seconds
    if (millis() - loopStartTime > 10000 && !haveResetCount) { 
      pressIndex = 0;
      haveResetCount = true;
      timeoutSound();
    }
  }

  long pressStartTime = millis();
  
  while(digitalRead(sensorPin)) { //press
    analogWrite(soundPin, volume);
  }

  digitalWrite(soundPin, LOW); 
  
  int pressTime = millis() - pressStartTime;
  
  pressTimes[pressIndex] = pressTime;
  Serial.println(String("[Press] index=") + pressIndex  + "  time=" + pressTime);
  
  haveResetCount = false;
  pressIndex++;

  if (pressIndex == numPresses) {
    bool stillCorrect = true;
    
    for (int i = 0; i < 4; i++) {
      if (abs(correctPressTimes[i]-pressTimes[i]) > thresh) {
        stillCorrect = false;
      }
    }

    if (!stillCorrect) { 
      errorSound();
    } else {
      unlock(pressColor);
    }

    pressIndex = 0;
    haveResetCount = true;
  }
}

void unlock(CRGB color) {
  
  succesSound();
  if (goUp) {
    Serial.println("[Dør] Gå op");
    turnOnLED(color);
  } else {
    Serial.println("[Dør] Gå ned");
    turnOffLED();
  }
  digitalWrite(enablePin,HIGH);
    
  for ( long i = 0; i < motorStepDistance; i++) {
    digitalWrite(pulsePin,HIGH);
    delayMicroseconds(50);
    digitalWrite(pulsePin,LOW);
    delayMicroseconds(50);
  }
  
  changeDirection();
}

void turnOnLED(CRGB color) {
  Serial.println("[LEDring] " );
  
  for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
      // Turn our current led on to white, then show the leds
      leds[whiteLed] = color;

      // Show the leds (only one of which is set to white, from above)
      FastLED.show();

      // Wait a little bit
      delay(10);

     
   }
}


void turnOffLED() {
  Serial.println("[LEDring] OFF");
  for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
      leds[whiteLed] = CRGB::Black;
      FastLED.show();
   }
}

void changeDirection() {
  if (goUp) {
    digitalWrite(directionPin,LOW);
    goUp = false;
  } else {
    digitalWrite(directionPin,HIGH);
    digitalWrite(enablePin, LOW); //release momentum
    goUp = true;
  }
}


void errorSound() {
  for (int i = 0; i < 20; i++) {
    analogWrite(soundPin, volume);
    delay(20);
    digitalWrite(soundPin, LOW);
    delay(20);
  }
}

void timeoutSound() {
  for (int i = 0; i < 3; i++) {
    analogWrite(soundPin, volume);
    delay(75);
    digitalWrite(soundPin, LOW);
    delay(75);
  }
}

void succesSound() { 
    //correct - unlock-sound
    for (int i = 0; i < 4; i++) {
      analogWrite(soundPin, volume);
      delay(200);
      digitalWrite(soundPin, LOW);
      delay(200);
    }
    analogWrite(soundPin, volume);
    delay(100);
    digitalWrite(soundPin, LOW);
    delay(100);
}

void checkRFID() {
  // Look for new cards
 if ( ! mfrc522.PICC_IsNewCardPresent())
 {
    delay(50);
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    delay(50);
    
    return;
    }
  }

  Serial.println("[RFID] NEW CARD");
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  String to_find = content.substring(1);
  int x; // generic loop counter
  
  for (x = 0; x < len; x++) {
    if (to_find == RFIDcards[x].macAdress && RFIDcards[x].access ) { 
      Serial.println("[RFID] Sucess med " + RFIDcards[x].desc + " " + RFIDcards[x].type + " (" + RFIDcards[x].macAdress + ")");
      unlock(RFIDcards[x].color);
      delay(10000);
      return;
    } 
  }
  Serial.println("[RFID] Fail with " + to_find );
  errorSound();
  delay(3000);
  
}

boolean hasUIDAssess (String to_find) {
  int len = (sizeof (RFIDcards) / sizeof (RFIDcards)); // how many elements in array
  int x; // generic loop counter
  for (x = 0; x < len; x++) {
    if (to_find == RFIDcards[x].macAdress && RFIDcards[x].access ) { 
      Serial.println("[RFID] Sucess med " + RFIDcards[x].desc + " " + RFIDcards[x].type + " (" + RFIDcards[x].macAdress + ")");
      return true;
    } 
  }
  Serial.println("[RFID] Fail med " + to_find);
  return false;
}
