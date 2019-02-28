/*
  Blink without Delay

  Turns on and off a light emitting diode (LED) connected to a digital pin,
  without using the delay() function. This means that other code can run at the
  same time without being interrupted by the LED code.

  The circuit:
  - Use the onboard LED.
  - Note: Most Arduinos have an on-board LED you can control. On the UNO, MEGA
    and ZERO it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN
    is set to the correct LED pin independent of which board is used.
    If you want to know what pin the on-board LED is connected to on your
    Arduino model, check the Technical Specs of your board at:
    https://www.arduino.cc/en/Main/Products

  created 2005
  by David A. Mellis
  modified 8 Feb 2010
  by Paul Stoffregen
  modified 11 Nov 2013
  by Scott Fitzgerald
  modified 9 Jan 2017
  by Arturo Guadalupi
  modified 2 Jan 2018
  by Changro Yoon

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/BlinkWithoutDelay
*/

// constants won't change. Used here to set a pin number:
const int ledPin =  LED_BUILTIN;// the number of the LED pin

// Variables will change:
int ledState = LOW;             // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

String inputString = "";
boolean stringComplete = false;

void setup() {
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);

  // establish serial communication
  Serial.begin(250000);
}

void loop() {
  // here is where you'd put code that needs to be running all the time.

  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.

  static long interval = 1000;           // interval at which to blink (milliseconds)
  unsigned long currentMillis = millis();
  static unsigned long currentMicros, previousMicros;

  currentMicros = micros();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);

    if (Serial){
      Serial.print("previous micros() : ");
      Serial.print(previousMicros);
      Serial.print(", current micros() : ");
      Serial.print(currentMicros);
      Serial.print(", loop interval : ");
      Serial.println(currentMicros-previousMicros);
    }

    // reroll random interval time (100~1000 milliseconds)
    interval = random(100,1001);
  }
  previousMicros = currentMicros;
  if (currentMillis < previousMillis) {
    Serial.println("Overflow detected !!!");
    Serial.println("Closing Serial Communication...");
    Serial.end();
  }
}

void serialEvent() {
  //Serial.println("Serial Event Triggered !!!");
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    //Serial.println(inChar);
    if (inChar == '\n') {
      //Serial.println("stringComplete Triggered !!!");
      stringComplete = true;
      inputString.trim();
      //Serial.println(inputString);
    }
  }
  if (stringComplete && inputString.startsWith("begin")) {
    Serial.println("starting begin()");

    Serial.end();
    Serial.begin(250000);
    
    inputString = "";
    stringComplete = false;
  }
  if (stringComplete && inputString.startsWith("end")) {
      Serial.println("starting end()");
      Serial.end();
      inputString = "";
      stringComplete = false;
  }
  if (stringComplete) {
    Serial.print("wrong command !!! : ");
    Serial.println(inputString);
    inputString = "";
    stringComplete = false;
  }
}

