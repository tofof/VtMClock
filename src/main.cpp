#include <Arduino.h>

/*******Interrupt-based Rotary Encoder Sketch*******
by Simon Merrett, based on insight from Oleg Mazurov, Nick Gammon, rt, Steve Spence
https://www.instructables.com/Improved-Arduino-Rotary-Encoder-Reading/
https://web.archive.org/web/20211022232828/https://www.instructables.com/Improved-Arduino-Rotary-Encoder-Reading/
*/

const static int pinA = 0, pinB = 1; // hardware interrupt pins for the rotary encoder
volatile byte aFlag = 0, bFlag = 0; // true when expecting a rising edge on corresponding pin to signal encoder arrived at detent (one is true depending on direction)
volatile byte encoderPos = 0, oldEncPos = 0; //value of current and previous encoder positions, 0-255 range is adequate for the 20-position encoder we're using
volatile uint32_t reading = 0; // holds raw read from the port register (faster and less code size than directRead)
const static EPortType port = g_APinDescription[pinA].ulPort; //pinB needs to be on the same port, verify this by checking if the returned porttypes are equal if changing pins
const static uint32_t pinMaskA = digitalPinToBitMask(pinA), pinMaskB = digitalPinToBitMask(pinB); //bitmasks for port register for the pins of interest
const static uint32_t pinMaskAB = pinMaskA + pinMaskB; //combined bitmask
void PinA(), PinB(); //forward declaration for interrupt processing functions

void setup() {
  pinMode(pinA, INPUT_PULLUP); // set pinA as an input, pulled HIGH to the logic voltage (3V3 in our case) so that no additional voltage needs to be supplied to the encoder
  pinMode(pinB, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(pinA),PinA,RISING); // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine 
  attachInterrupt(digitalPinToInterrupt(pinB),PinB,RISING);
  Serial.begin(115200);  
}

void PinA(){
  noInterrupts(); //stop interrupts happening while we're inside interrupt handler
  reading = PORT->Group[port].IN.reg & pinMaskAB;
  if(reading == pinMaskAB && aFlag) { //are we at detent (both pins HIGH) and we're expecting detent on this pin's rising edge?
    encoderPos --; //decrement the encoder's position count
    aFlag = bFlag = 0; //reset flags
  }
  else if (reading == pinMaskA) { //pinB is not yet HIGH
    bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
  } 
  interrupts(); //restart interrupts
}

void PinB(){ //corresponds to PinA(), just other pin
  noInterrupts(); 
  reading = PORT->Group[port].IN.reg & pinMaskAB;
  if (reading == pinMaskAB && bFlag) { //difference from PinA(): a->b
    encoderPos ++; //difference from pinA(): decrement -> increment
    aFlag = bFlag = 0; 
  }
  else if (reading == pinMaskB) { //difference from PinA(): a->b
    aFlag = 1; //difference from PinA(): b->a
  } 
  interrupts(); 
}

void loop(){
  if(oldEncPos != encoderPos) {
    Serial.println(encoderPos);
    oldEncPos = encoderPos;
  }
  
}
