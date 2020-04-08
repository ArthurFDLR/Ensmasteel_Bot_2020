// =============================
// ===       Libraries       ===
// =============================


#include <Arduino.h>
#include "Communication.h"
#include "Actuators.h"
#include "Actuators_Manager.h"

#define DELAY 10
uint32_t lastMillis = 0;

Manager* manager;

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);
  Serial.println("STARTED");
  manager = new Manager(&Serial2, &Serial);
  lastMillis = millis();
}

void loop()
{
  if (millis() - lastMillis > DELAY)
  {
    lastMillis = millis();

    manager->Update();
  }
}



/*
#include <Arduino.h>

// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS 200
#define RPM 250

#define DIR 35
#define STEP 34
#define SLEEP 22 // optional (just delete SLEEP from everywhere if not used)

#include "DRV8834.h"
#define M0 37
#define M1 36
DRV8834 stepper(MOTOR_STEPS, DIR, STEP, SLEEP, M0, M1);

// #include "A4988.h"
// #define MS1 10
// #define MS2 11
// #define MS3 12
// A4988 stepper(MOTOR_STEPS, DIR, STEP, SLEEP, MS1, MS2, MS3);

// #include "DRV8825.h"
// #define MODE0 10
// #define MODE1 11
// #define MODE2 12
// DRV8825 stepper(MOTOR_STEPS, DIR, STEP, SLEEP, MODE0, MODE1, MODE2);

// #include "DRV8880.h"
// #define M0 10
// #define M1 11
// #define TRQ0 6
// #define TRQ1 7
// DRV8880 stepper(MOTOR_STEPS, DIR, STEP, SLEEP, M0, M1, TRQ0, TRQ1);

// #include "BasicStepperDriver.h" // generic
// BasicStepperDriver stepper(DIR, STEP);

void setup() {
    stepper.begin(RPM);
    // if using enable/disable on ENABLE pin (active LOW) instead of SLEEP uncomment next line
    // stepper.setEnableActiveState(LOW);
    stepper.enable();
    stepper.setMicrostep(8);  // Set microstep mode to 1:1
    // set current level (for DRV8880 only). 
    // Valid percent values are 25, 50, 75 or 100.
    // stepper.setCurrent(100);
}

void loop() {
    // One complete revolution is 360°
    stepper.enable();
    stepper.rotate(360*2);     // forward revolution
    //stepper.rotate(-360);    // reverse revolution
    stepper.disable();
    delay(500);
    //
}
*/