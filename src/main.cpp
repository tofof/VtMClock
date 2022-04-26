#include <Arduino.h>

/*******Interrupt-based Rotary Encoder Sketch*******
by Simon Merrett, based on insight from Oleg Mazurov, Nick Gammon, rt, Steve Spence
*/

const static int pinA = 2; // Our first hardware interrupt pin is digital pin 2
const static int pinB = 3; // Our second hardware interrupt pin is digital pin 3
volatile byte aFlag = 0; // lets us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0; // lets us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile byte encoderPos = 0; //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte if you want to record a larger range than 0-255
volatile byte oldEncPos = 0; //stores the last encoder position value so we can compare to the current reading and see if it has changed (so we know when to print to the serial monitor)
volatile int stateA = 0; //somewhere to store the raw value we read off the pin
volatile int stateB = 0;

void PinA();
void PinB();

void setup() {
  pinMode(pinA, INPUT_PULLUP); // set pinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  pinMode(pinB, INPUT_PULLUP); // set pinB as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  attachInterrupt(digitalPinToInterrupt(pinA),PinA,RISING); // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (below)
  attachInterrupt(digitalPinToInterrupt(pinB),PinB,RISING); // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (below)
  Serial.begin(115200); // start the serial monitor link
}

void PinA(){
  noInterrupts(); //stop interrupts happening before we read pin values
  stateA = digitalRead(pinA);
  stateB = digitalRead(pinB);
  if(stateA == HIGH && stateB == HIGH) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos --; //decrement the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (stateA == HIGH && stateB == LOW) bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
  interrupts(); //restart interrupts
}

void PinB(){
  noInterrupts(); //stop interrupts happening before we read pin values
  stateA = digitalRead(pinA);
  stateB = digitalRead(pinB);
  if (stateA == HIGH && stateB == HIGH && bFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos ++; //increment the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (stateA == LOW && stateB == HIGH) aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
  interrupts(); //restart interrupts
}

void loop(){
  if(oldEncPos != encoderPos) {
    Serial.println(encoderPos);
    oldEncPos = encoderPos;
  }
}
