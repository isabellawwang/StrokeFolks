#include <string.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LiquidCrystal_I2C.h>
#include <TimerOne.h>
#include "TM1637.h"
#define ON 1
#define OFF 0

// Clock Setup
int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};
unsigned char ClockPoint = 1;
unsigned char Update;
unsigned char halfsecond = 0;
unsigned char second;
unsigned char minute = 30;
unsigned char hour = 12;


#define CLK 5//pins definitions for TM1637 and can be changed to other ports
#define DIO 6
TM1637 tm1637(CLK,DIO);

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// EEPROM VARIABLES
struct personProfile {
  int baseline;
  int userID;
  int avaliable;
};
personProfile guest = {5, 1, 0};
personProfile current_profile;

// Button Pins
int buttonUp = 2;
int buttonDown = 3;
int buttonEnter = 8;

// LED pins
int RedLED = 49;
int GreenLED = 51;

// Speaker pins
int speakerPin = 10;

// Flags setup
int initializeFlag = 0;
int frequency = 0; // stimulation frequency
int frequencyFlag = 0;
int frequencyMessage;
int baselineNumber = '5';
int baselineFlag = 0;

void setup() {

  // Store Guest Profile
  storeProfileEEPROM(guest);

  // Start LCD
  lcd.begin();

  displayOnLCD("Welcome to your Stroke Alarm!"); // Welcoming Message

  delay(1000);

  // initialize buttons
  pinMode(buttonUp, INPUT);
  digitalWrite(buttonUp, HIGH);
  pinMode(buttonDown, INPUT);
  digitalWrite(buttonDown, HIGH);
  pinMode(buttonEnter, INPUT);
  digitalWrite(buttonEnter, HIGH);

  // Initialize LEDS
  pinMode(GreenLED, OUTPUT);
  pinMode(RedLED, OUTPUT);

  // Initialize Speakers
  pinMode(speakerPin, OUTPUT);

  // Adjust Clock
  setTime();

  // Clock Initialization
  tm1637.set();
  tm1637.init();
  Timer1.initialize(500000);//timing for 500ms
  Timer1.attachInterrupt(TimingISR);//declare the interrupt serve routine:TimingISR

  // initialize both serial ports:
  Serial.begin(9600);
  Serial1.begin(38400);

  // initialize bluetooth
  initializeBT();

  // Feedback to user
  displayOnLCD("Ready to Roll!");

  // Profile Choice
  displayOnLCD("Let's choose your profile");
  profileSelection(); // Start Protocol to choose profile

  displayOnLCD("Press enter to start Night Monitoring");
  while (digitalRead(buttonEnter) == HIGH) {
  }

  displayOnLCD("Let's go!");
  delay(1000);
}

void loop() {

  // For Digital Clock 
  if(Update == ON) // Updates Time
  {
    TimeUpdate();
    tm1637.display(TimeDisp);
  }
  
  // Test BT Communication
  testBT(); // Uncomment if you need to test BT connection

  // Start Stimulation Routine
  stimulationRoutine();

}

void profileSelection(){

  // Ask if you have a profile
  lcd.clear();
  displayOnLCD("If you have a profile, press up.");
  delay(1000);
  displayOnLCD("If you don't, press down.");
  delay(1000);

  int haveIDChoice = 0;
  current_profile = guest;

  while (1) {

    if (digitalRead(buttonDown) == LOW) { // Pressed down, no profile 
      displayOnLCD("To make a new profile, press up.");
      displayOnLCD("Otherwise you will continue as a guest");
      unsigned long stTime = millis();
      int profile_created = 0;
      while (millis() - stTime < 20000) {
        if (digitalRead(buttonUp) == LOW) {
          current_profile = makeNewProfile();
          break;
        }
      }
      if (current_profile.userID == guest.userID) {
        displayOnLCD("Using guest profile.");
        
      }
      break;
    }

    if (digitalRead(buttonUp) == LOW) { // Have profile
      int id_selected = 0;

      displayOnLCD("Press up to increment your ID,");
      displayOnLCD("and press down to decrement it.");
      displayOnLCD("To select one, press enter.");
      id_selected = chooseID();

      displayOnLCD("Selected ID: ");
      lcd.print(id_selected);
      displayOnLCD("Retrieving profile...");

      current_profile = retrieveProfileEEPROM(id_selected);
      break;
    }

  }

  if (current_profile.avaliable != 0) { // Profile not available
    displayOnLCD("Invalid ID provided.");
    // would you like to continue as a guest, or make a profile?
  }

  delay(200);
  lcd.clear();
  lcd.print("Valid user found!");

}

void stimulationRoutine(){

  /*
  1. Send initialization flag (s)
  2. Get (c) and move on
  3. Send Baseline Recording Flag (b)
  4. Get (r) and move on. Or just show baseline?
  5. Send frequency of stimulation (h - 30 seconds, w - minute, t - 30 minutes)
  6. Get random number and compare with baseline.
  7. If lower than baseline, send (l), and sound the alarm. RED LED
  8. If higher than baseline, send (g), and green LED
  9. Wait for random number
  */

  chooseFrequency();

  displayOnLCD("Frequency chosen!");
  delay(1000);

  comparisonLoop();

}

void comparisonLoop(){
  int control = 0;
  while(1){

  // For Digital Clock 
  if(Update == ON) // Updates Time
  {
    TimeUpdate();
    tm1637.display(TimeDisp);
  }

  while(Serial1.available() == 0)
  {
    displayOnLCD("Waiting for stimulation...");

    if(Update == ON) // Updates Time
    {
    TimeUpdate();
    tm1637.display(TimeDisp);
    }

    control = 0;
  }

  displayOnLCD("Receiving Data...");

  int inByte = Serial1.read(); // Read incoming Data

  Serial.write(inByte); // What was the data read?

  if((inByte < '5') && (control == 0)) // Smaller than baseline
  {
    Serial1.write("l"); // Alert Arm
    emergencyResponse(); // Quick response
    control = 1;
  }

  if((inByte >= '5')) // Larger than baseline
  {
    Serial1.write("g"); // Alert Arm
    displayOnLCD("Passed");
    delay(1000);
    control = 1;
  }

  if(control == 3)
  {
    control = 0;
  }
  
  // lcd.write(inByte); // Option to display stuff

  delay(1000);

  }
}

void emergencyResponse(){ 
  // For now, as soon as there is a message below the baseline, the clock will stop exactly when there was an alert. This can be changed by adding the update time lines below.

  while((digitalRead(buttonUp) == HIGH) && (digitalRead(buttonDown) == HIGH) && (digitalRead(buttonEnter) == HIGH)) // For now, after one presses a button, it just goes back to the loop above.
  {
    // Turn Green LED off
  digitalWrite(GreenLED, LOW);

    // Turn Red LED on
  digitalWrite(RedLED, HIGH);

  // Alarm on
  tone(speakerPin, 1000, 2000);

  // Turn Red LED on
  digitalWrite(RedLED, LOW);

  // Display Alert
  displayOnLCD("STROKE ALERT!");

  // Turn Red LED on
  digitalWrite(RedLED, HIGH);

  // Press any button to call someone (future feature)
  lcd.setCursor(0,1);
  lcd.print("Press to Call");

  delay(500);

  }
}

void chooseFrequency(){

  displayOnLCD("Choose Stimulation Frequency");
  delay(1000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Up 30s, Down 1m,");
  lcd.setCursor(0,1);
  lcd.print("Enter 30m");
  //displayOnLCD("Up for 30s, Down for 1m, Enter for 30m");

  while(1)
  {
    // Up for 30s
    if(digitalRead(buttonUp) == LOW)
    {
      displayOnLCD("30 seconds");
      delay(1000);
      frequency = 30; //seconds
      frequencyMessage = 'h';
      Serial1.write(frequencyMessage);
      break;
    }

    // Down for 1 minute
    if(digitalRead(buttonDown) == LOW)
    {
      displayOnLCD("1 minute");
      delay(1000);
      frequency = 60; //seconds
      frequencyMessage = 'w';
      Serial1.write(frequencyMessage);
      break;
    }

    // Enter for 30 minutes
    if(digitalRead(buttonEnter) == LOW)
    {
      displayOnLCD("30 minutes");
      delay(1000);
      frequency = 1800; //seconds
      frequencyMessage = 't';
      Serial1.write(frequencyMessage);
      break;
    }
  }

  displayOnLCD("Loading...");

  while(frequencyFlag != 1) // Loop sending messages to the other side until something comes back
    {
      // Serial1.write(frequencyMessage);
      if(Serial1.available())
      {
        int inByte = Serial1.read();

        if(inByte == 'j')
        {
          displayOnLCD("Received!");
          delay(1000);
          frequencyFlag = 1;
        }
      }

    }
}

void testBT(){

  int buttonState = digitalRead(buttonEnter);

  //read from port 1, send to port 0:
  if (Serial1.available()) {

    int inByte = Serial1.read();

    if(inByte == 'H')
    {
      Serial1.write("e");
    }

    if(inByte == 'o')
    {
      Serial1.write("s");
    }

    Serial.write(inByte);
    // lcd.write(inByte);
    delay(1000);

  }

  // read from port 0, send to port 1:
  if (Serial.available()) {

    int inByte = Serial.read();
    
    Serial1.write(inByte);

  }

  if(buttonState == 0)
  {
    Serial1.write("1");
  }

  delay(200); // avoid button debouncing

}

void initializeBT(){

  // Serial1.write("s");
  displayOnLCD("Initializing...");

  while(initializeFlag != 1) // Loop sending messages to the other side until something comes back
  {
    Serial1.write("s");


    if(Serial1.available())
    {
      int inByte = Serial1.read();

      if(inByte == 'c')
      {
        displayOnLCD("Working!");
        initializeFlag = 1;
      }
    }

  }
  
}

void setTime(){

   // hour setup
    displayOnLCD("Set the hour");

    while(digitalRead(buttonEnter) != LOW)
    {
      lcd.setCursor(0,1);
      lcd.print("Hour:     ");
      lcd.setCursor(6,1);
      lcd.print(hour);

      if((digitalRead(buttonUp) == LOW))
      { 
        if(hour == 23)
        {
          hour = 0;
        }
        else{
          hour = hour + 1;
        }
      }

      if((digitalRead(buttonDown) == LOW))
      {
        if(hour == 0)
        {
          hour = 23;
        }
        else{
          hour = hour - 1;
        }
      }

      delay(150);

    }

    delay(300);

    // minute setup
    displayOnLCD("Set the minute");

    while(digitalRead(buttonEnter) != LOW)
    {
      lcd.setCursor(0,1);
      lcd.print("Minute:    ");
      lcd.setCursor(8,1);
      lcd.print(minute);

      if((digitalRead(buttonUp) == LOW))
      { 
        if(minute == 60)
        {
          minute = 0;
        }
        else{
          minute = minute + 1;
        }
      }

      if((digitalRead(buttonDown) == LOW))
      {
        if(minute == 0)
        {
          minute = 60;
        }
        else{
          minute = minute - 1;
        }
      }
      delay(150);
    }

    delay(200);

}

void TimingISR(){
  halfsecond ++;
  Update = ON;
  if(halfsecond == 2){
    second ++;
    if(second == 60)
    {
      minute ++;
      if(minute == 60)
      {
        hour ++;
        if(hour == 24)hour = 0;
        minute = 0;
      }
      second = 0;
    }
    halfsecond = 0;
  }
 // Serial.println(second);
  ClockPoint = (~ClockPoint) & 0x01;
}

void TimeUpdate(void){
  if(ClockPoint)tm1637.point(POINT_ON);
  else tm1637.point(POINT_OFF);
  TimeDisp[0] = hour / 10;
  TimeDisp[1] = hour % 10;
  TimeDisp[2] = minute / 10;
  TimeDisp[3] = minute % 10;
  Update = OFF;
}

void displayOnLCD(char *text) {
  lcd.clear();
  int length = strlen(text);
  if (length < 16)
    lcd.print(text);
  else {
    int pos;
    for (pos = 0; pos < 16; pos++)
      lcd.print(text[pos]);
    delay(2000);
    while (pos < length) {
      lcd.scrollDisplayLeft();
      lcd.print(text[pos]);
      pos = pos + 1;
      delay(300);
    }
  }
}

int chooseID() {

  int id_count = 0;
  int chosen = 0;
  lcd.clear();

  while (chosen == 0) {

    int incrementRead = !digitalRead(buttonUp);
    int decrementRead = !digitalRead(buttonDown);
    int chosen = !digitalRead(buttonEnter);
    lcd.setCursor(0, 0);
    lcd.print("Current id: ");
    lcd.print(id_count);
    lcd.setCursor(0,0);

    if (incrementRead) {
      if (id_count < 10) {
        id_count++;
        lcd.print("Current id: ");
        lcd.setCursor(12,0);
        lcd.print(id_count);
      }
    } 
    
    else if (decrementRead) {
      if (id_count > 0) {
        id_count--;
        lcd.print("Current id: ");
        lcd.setCursor(12,0);
        lcd.print(id_count);
      }
    } 
    
    else if (chosen) {
      break;
    }

    delay(200);

  }

  return id_count;

}

void storeProfileEEPROM(personProfile profileToStore) {
  EEPROM.put(profileToStore.userID * sizeof(personProfile), profileToStore);
}

personProfile retrieveProfileEEPROM(int userID) {
  personProfile retrievedProfile;
  EEPROM.get(userID * sizeof(personProfile), retrievedProfile);
  return retrievedProfile;
}

personProfile makeNewProfile() {
  personProfile newProfile;
  personProfile profileToCheck;
  for (int i = 1; i < 10; i++) {
    EEPROM.get(i * sizeof(personProfile), profileToCheck);
    Serial.println(profileToCheck.avaliable);
    if (profileToCheck.avaliable != 0) {

      newProfile = profileToCheck;
      newProfile.userID = i;

      displayOnLCD("Profile num: ");
      lcd.print(i);
      delay(5000);
      newProfile.avaliable = 0;

      //Connect to get baseline. In this case, I will just flag to the arm. The arm will wait for a button press. If the button is pressed, it will flag back, and the baseline will be x.
      displayOnLCD("Starting baseline test...");

      Serial1.write("b");

      while(baselineFlag != 1) // Loop sending messages to the other side until something comes back
      {
        // displayOnLCD("Waiting for baseline test...");

        if(Serial1.available())
        {P
          int inByte = Serial1.read();

          if(inByte == 'r')
          {
            displayOnLCD("Baseline Recorded");
            baselineFlag = 1;
            newProfile.baseline = 5;
          }
        }

      }

      
      
      return newProfile;
    }
  }
  return guest;
}
