/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara
    Refactored back to IDF by H2zero

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/

/** NimBLE differences highlighted in comment blocks **/

/*******original********
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
***********************/

#include "Arduino.h"
#include "bleScript.cpp"
#include "Accelerometer.cpp"
#include "capacitiveSensing.cpp"
#include "Wire.h"


#define LDO_GPIO GPIO_NUM_3 
#define LED_GPIO GPIO_NUM_6
#define BAT_VOL_GPIO GPIO_NUM_4
#define BUTTON_GPIO GPIO_NUM_5


unsigned long previousMillis_LED = 0;        // Stores the last time the LED was updated
const long interval_LED = 3000;              // Interval at which to blink (milliseconds)
const long onDuration_LED = 10;      // Duration to keep the LED on (milliseconds)
bool stateLED = LOW;             // Current state of the LED

unsigned long previousMillis_bat = 0;        // Stores the last time the LED was updated
const long interval_bat = 5000;              // Interval at which to blink (milliseconds)

// Global var
uint32_t value = 0;
bool buttonPressed = 0;
float batVoltage = 0;
long capSensVal0 = 0;
long capSensVal1 = 0;
float acc_x_g = 0;
float acc_y_g = 0;
float acc_z_g = 0;



void blinkLED(void){
    if (pCharacteristicLED->getValue<uint8_t>() != 0)
    {
        unsigned long currentMillis = millis();

        if (stateLED == LOW && currentMillis - previousMillis_LED >= interval_LED) {
            // It's time to turn the LED on
            previousMillis_LED = currentMillis;  // Save the current time
            digitalWrite(LED_GPIO, HIGH);      // Turn the LED on
            stateLED = HIGH;                 // Update the state
        }

        if (stateLED == HIGH && currentMillis - previousMillis_LED >= onDuration_LED) {
            // It's time to turn the LED off
            digitalWrite(LED_GPIO, LOW);       // Turn the LED off
            stateLED = LOW;                  // Update the state
        }
    }
    else{
        digitalWrite(LED_GPIO, LOW);
    }
}

// Reads the ADC of the Battery voltage and turns into (V)
void updateBatVol(void){
    // Current time in milliseconds
    unsigned long currentMillis = millis();
     // Check if it's time to blink the LED
    if (currentMillis - previousMillis_bat >= interval_bat) {
        //Serial.println("Update Batt Voltage");
        // Save the last time it updated
        previousMillis_bat = currentMillis;
        // The voltage divider with 1M Ohm and 249K x analog value x conversion factor
        batVoltage = ((1000.0+249.0)/249.0)*analogRead(BAT_VOL_GPIO) * 2.75 / 4095.0; 
  }
}


void setup(void){
    Serial.begin(115200);

    // SET THE LDO TO RUN ALONE
    pinMode(LDO_GPIO, OUTPUT);
    digitalWrite(LDO_GPIO, HIGH); 

    // SET THE TEST LED TO RUN HIGH 
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, HIGH); 
    Serial.println("LDO Set high");

     // SET THE BATTERY VOLTAGE MEASUREMENT
    pinMode(BUTTON_GPIO, INPUT);

    // Start BLE
    setupBLE();

    // Setup Accelerometer MC3635
    setupMC3635();

    // Init CapcitiveSesning
    setupCapacitiveSensing();
}

void loop(){
    
    blinkLED();
    updateBatVol();

    //buttonPressed = digitalRead(BUTTON_GPIO); // Reads if the button is pressed (NOT USED YET)
    runCapcitiveSensing(&capSensVal0, &capSensVal1);
    readMC3635(&acc_x_g, &acc_y_g, &acc_z_g);

    // Update all values to the BLE and notify
    updateBLE(value,batVoltage,capSensVal0,capSensVal1, acc_x_g, acc_y_g, acc_z_g);

    //value++; // Test value that goes up
    delay(200);
}

