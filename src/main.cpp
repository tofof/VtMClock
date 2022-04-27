#include <Arduino.h>
#include <TM1637.h>

/*
Interrupt-based Rotary Encoder Sketch
by Simon Merrett, based on insight from Oleg Mazurov, Nick Gammon, rt, Steve Spence
https://www.instructables.com/Improved-Arduino-Rotary-Encoder-Reading/
https://web.archive.org/web/20211022232828/https://www.instructables.com/Improved-Arduino-Rotary-Encoder-Reading/
*/

/*
TM1637 Digit Display - Arduino Quick Tutorial
by Ryan Chan
https://create.arduino.cc/projecthub/ryanchan/tm1637-digit-display-arduino-quick-tutorial-ca8a93
https://web.archive.org/web/20220111231154/https://create.arduino.cc/projecthub/ryanchan/tm1637-digit-display-arduino-quick-tutorial-ca8a93
*/

//rotary encoder
const static int aPin = 0, bPin = 1; // hardware interrupt pins for the rotary encoder
volatile byte aFlag = 0, bFlag = 0; // true when expecting a rising edge on corresponding pin to signal encoder arrived at detent (one is true depending on direction)
volatile byte encoderPos = 78, oldEncPos = 0; //value of current and previous encoder positions, 0-255 range is adequate for the 20-position encoder we're using, 78 will correspond to 06:30 on the clock
volatile uint32_t reading = 0; // holds raw read from the port register (more performant than directRead)
const static EPortType port = g_APinDescription[aPin].ulPort; //pinB needs to be on the same port, verify this by checking if the returned porttypes are equal if changing pins
const static uint32_t pinMaskA = digitalPinToBitMask(aPin), pinMaskB = digitalPinToBitMask(bPin); //bitmasks for port register for the pins of interest
const static uint32_t pinMaskAB = pinMaskA + pinMaskB; //combined bitmask
void aIRQ(), bIRQ(), buttonIRQ(); //forward declaration for interrupt processing functions

//encoder pushbutton
const int buttonPin = 2; // the number of the pushbutton pin
const int ledPin =  13; // the number of the LED pin
const static uint32_t pinMaskButton = digitalPinToBitMask(buttonPin);
volatile uint32_t buttonState = 0;

//7-segment display
const static int clkPin = 4, dioPin = 5; //signal pins for the digit display
TM1637 tm(clkPin,dioPin); //using the library for the 7-segment display

void setup() {
  pinMode(aPin, INPUT_PULLUP); // set pinA as an input, pulled HIGH to the logic voltage (3V3 in our case) so that no additional voltage needs to be supplied to the encoder
  pinMode(bPin, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(aPin),aIRQ,RISING); // set an interrupt on pinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine 
  attachInterrupt(digitalPinToInterrupt(bPin),bIRQ,RISING);

  pinMode(ledPin, OUTPUT); //testing pushbutton
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin),buttonIRQ,CHANGE);

  tm.set(7); //7-segment brightness (0-7)
  Serial.begin(115200);
}

//displays a number[0-9999] as 4 digits
void displayNumber(int num) {   
    tm.display(3, num % 10);   
    tm.display(2, num / 10 % 10);   
    tm.display(1, num / 100 % 10);   
    tm.display(0, num / 1000 % 10);
}

//display minutes:seconds based on elapsed seconds
void displayTimeMMSS(int secondsElapsed) {
    int minutes = secondsElapsed / 60;
    int seconds = secondsElapsed % 60;

    tm.point(1); //colon
    tm.display(3, seconds % 10);
    tm.display(2, seconds / 10 % 10);
    tm.display(1, minutes % 10);
    tm.display(0, minutes / 10 % 10);
}

//display hours:minutes based on elapsed minutes; will display 00 hours for elapsed time, or use range 60-779 to correspond to 1:00-12:59 for valid clock times
void displayTimeHHMM(int minutesElapsed) {
    int hours = minutesElapsed / 60;
    int minutes = minutesElapsed % 60;

    tm.point(1); //colon
    tm.display(3, minutes % 10);
    tm.display(2, minutes / 10 % 10);
    tm.display(1, hours % 10);
    tm.display(0, hours / 10 % 10);
}

//interrupt handler
void aIRQ() {
  noInterrupts(); //stop interrupts happening while we're inside interrupt handler
  reading = PORT->Group[port].IN.reg & pinMaskAB;
  if(reading == pinMaskAB && aFlag) { //are we at detent (both pins HIGH) and we're expecting detent on this pin's rising edge?
    encoderPos --; //decrement the encoder's position count
    aFlag = bFlag = 0; //reset flags
  }
  else if (reading == pinMaskA) { //bPin is not yet HIGH
    bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
  } 
  interrupts(); //restart interrupts
}

//interrupt handler; corresponding function of aIRQ()
void bIRQ() { 
  noInterrupts(); 
  reading = PORT->Group[port].IN.reg & pinMaskAB;
  if (reading == pinMaskAB && bFlag) { //difference from aIRQ(): a->b
    encoderPos ++; //difference from aIRQ(): decrement -> increment
    aFlag = bFlag = 0; 
  }
  else if (reading == pinMaskB) { //difference from aIRQ(): a->b
    aFlag = 1; //difference from aIRQ(): b->a
  } 
  interrupts(); 
}

void buttonIRQ() {
  noInterrupts(); 
  buttonState = (PORT->Group[port].IN.reg & pinMaskButton) == pinMaskButton;
  interrupts();
}

void loop() {
  if(oldEncPos != encoderPos) {
    Serial.println(encoderPos);
    oldEncPos = encoderPos;
    if(encoderPos >= 156) { //156*5=780 minutes i.e. 13 hours
      encoderPos=12;  //display 13:00 time as 1:00, i.e. (1*60+0)/5=12
    }
    if(encoderPos <=11) { //11*5=55 minutes i.e. less than 1 hour
      encoderPos=155; //display 00:55 time as 12:55, i.e. (12*60+55)/5=155
    }
  }

  if(buttonState == HIGH) {
    digitalWrite(ledPin, HIGH);
  }
  else {
    digitalWrite(ledPin, LOW);
  }

  displayTimeHHMM(encoderPos*5);  //each detent is 5 minutes
}
