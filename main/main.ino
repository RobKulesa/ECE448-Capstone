#include <Arduino.h>
#include <HardwareSerial.h>
#include <Math.h>
#include <Servo.h>

// Define global variables
uint8_t firstImpulse = 0;
uint8_t secondImpulse = 0;
uint8_t thirdImpulse = 0;
uint32_t firstTimeStamp = 0;
uint32_t secondTimeStamp = 0;
uint8_t pos = 0;
const double micDistance = 15; //inches
const double speedOfSound = 0.0133; //inches per microsecond
Servo servo;

// Define macro function for updating global variables upon interrupt events
#define ISR_MACRO(pin) {\
  if(firstImpulse == 0) {\
    firstTimeStamp = micros();\
    firstImpulse = pin;\
  } else if(secondImpulse == 0) {\
    noInterrupts();\
    secondTimeStamp = micros();\
    secondImpulse = pin;\
  }\
}

// Define hardware-arduino connections
const uint8_t gatePins[] = {21, 3, 18};
const uint8_t servoPin = 13;

// Interrupt Service Routines specific to the sound detector gate pins
void soundISR0() {
  ISR_MACRO(gatePins[0]);
}
void soundISR1() {
  ISR_MACRO(gatePins[1]);
}
void soundISR2() {
  ISR_MACRO(gatePins[2]);
}

// Set up the sound detector pins for input and attach respective ISRs upon digital rising edge
// and begin serving requests for interrupts
void setup() {
  Serial.begin(9600);
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(gatePins[i], INPUT);
  }
  attachInterrupt(digitalPinToInterrupt(gatePins[0]), soundISR0, RISING);
  attachInterrupt(digitalPinToInterrupt(gatePins[1]), soundISR1, RISING);
  attachInterrupt(digitalPinToInterrupt(gatePins[2]), soundISR2, RISING);
  servo.attach(servoPin);
  interrupts();
  Serial.println("Ready to serve requests");
  servo.write(0);
}

// When two impulses are logged, print the time difference between the first and second impulse
// and reset the global variables
void loop() { 
  while(secondImpulse == 0) {
    Serial.println("Waiting for requests");
    delay(1000);
  }
  uint32_t timeDiff;
  if(firstTimeStamp > secondTimeStamp) timeDiff = UINT32_MAX - firstTimeStamp + secondTimeStamp;
  else timeDiff = secondTimeStamp - firstTimeStamp;

  double ratio = (timeDiff * speedOfSound) / micDistance;
  if(ratio <= 1) {
    uint16_t theta_rel = (180/M_PI) * asin(ratio);
    uint16_t theta_abs = angleOffset(theta_rel);
    char buffer[150];
    sprintf(buffer, "Impulse 1: %hu\nImpulse 2: %hu\nTime difference (us): %lu\nTheta: %hu", firstImpulse, secondImpulse, timeDiff, theta_abs);
    Serial.println(buffer);
    writeServo(theta_abs / 2);
    delay(2000);
  }
  clearLog();
  interrupts();
}

void clearLog() {
  firstImpulse = 0;
  secondImpulse = 0;
  firstTimeStamp = 0;
  secondTimeStamp = 0;
}

uint16_t angleOffset(uint16_t theta_rel) {
  if(firstImpulse == gatePins[0] && secondImpulse == gatePins[1]) {
    return 60 - theta_rel;
  } else if(firstImpulse == gatePins[1] && secondImpulse == gatePins[0]) {
    return 60 + theta_rel;
  } else if(firstImpulse == gatePins[1] && secondImpulse == gatePins[2]) {
    return 180 - theta_rel;
  } else if(firstImpulse == gatePins[2] && secondImpulse == gatePins[1]) {
    return 180 + theta_rel;
  } else if(firstImpulse == gatePins[2] && secondImpulse == gatePins[0]) {
    return 300 - theta_rel;
  } else { //firstImpulse == gatePins[0] && secondImpulse == gatePins[2]
    return 300 + theta_rel;
  }
}

void writeServo(uint16_t theta) {
  if(labs(servo.read() - theta) > 5) {
      servo.write(theta);
  }
}
