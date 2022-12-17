// By Bryan Gonzalez

// Speaker Pin
int SpeakerPin = 33;

// LED pin
int GreenLED = 0;
int RedLED = 0;

// Button Pin
int buttonPin = 6;

// Stimulation Frequency
int frequency = 0;

// Time flags
int startTime = 0;
int endTime = 0;
int currentTime = 0;
int bufferTime = 0;

// Stimulation Value to be sent
long stimulationValue = 0;

void setup() {

  // initialize both serial ports:

  Serial.begin(9600);

  Serial1.begin(38400);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SpeakerPin, OUTPUT);

  pinMode(GreenLED, OUTPUT);
  pinMode(RedLED, OUTPUT);

  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);


}

void loop() {

  // Check times
  currentTime = millis()/1000;

  dataInSerial1();

  dataInSerial();

  endTime = currentTime - startTime;

  if ((endTime == frequency) && startTime != 0)
  {
    stimulationValue = random(0,11);

    switch (stimulationValue){
      case 0:
        Serial1.write("0");
        break;
      case 1:
        Serial1.write("1");
        break;
      case 2:
        Serial1.write("2");
        break;
      case 3:
        Serial1.write("3");
        break;
      case 4:
        Serial1.write("4");
        break;
      case 5:
        Serial1.write("5");
        break;
      case 6:
        Serial1.write("6");
        break;
      case 7:
        Serial1.write("7");
        break;
      case 8:
        Serial1.write("8");
        break;
      case 9:
        Serial1.write("9");
        break;
      case 10:
        Serial1.write("10");
        break;
    }

    digitalWrite(GreenLED, HIGH);
    startTime = 0;
    // bufferTime = currentTime;
    delay(1000);
    digitalWrite(GreenLED, LOW);

  }

}

void dataInSerial1(){

  // read from port 1, send to port 0:
  if (Serial1.available()) { // if there is data from BT

    int inByte = Serial1.read(); // read the data

     // Initialization protocol
    if(inByte == 's') // if it is 's', send 'c'
    {
      Serial1.write("c");
    }

    // Frequency Selection - 30 seconds
    if(inByte == 'h') // if it is 's', send 'c'
      {
        frequency = 30;
        Serial1.write("j");
        startTime = millis()/1000;
      }

    // Frequency Selection - 1 minute
    if(inByte == 'w') // if it is 's', send 'c'
      {
        frequency = 60;
        Serial1.write("j");
        startTime = millis()/1000;
      }

    // Frequency Selection -30 minutes
    if(inByte == 't') // if it is 's', send 'c'
      {
        frequency = 1800;
        Serial1.write("j");
        startTime = millis()/1000;
      }
    
    // Bad Result
    if(inByte == 'l') // if it is 's', send 'c'
      {
        digitalWrite(RedLED, HIGH);
        tone(SpeakerPin, 1000, 2000);
      }

    // Good Result
    if(inByte == 'g') // if it is 's', send 'c'
      {
        digitalWrite(GreenLED, HIGH);
        delay(1000);
        digitalWrite(GreenLED, LOW);
        startTime = millis()/1000;
      }
    
    if(inByte == 'b') // if it is 's', send 'c'
      {
        digitalWrite(GreenLED, HIGH);
        
        while(digitalRead(buttonPin) == HIGH);

        Serial1.write("r");

        digitalWrite(GreenLED, LOW);
      }

    Serial.write(inByte);  //show what was read
    
  }

}

void dataInSerial()
{
  // read from port 0, send to port 1:
    // Basically, code to send stuff
    if (Serial.available()) {

      int inByte = Serial.read();

      Serial1.write(inByte);

    }
}
