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

#define CSENS0_GPIO GPIO_NUM_0 
#define CSENS1_GPIO GPIO_NUM_1 
#define CGENERATE_GPIO GPIO_NUM_7 


void setupCapacitiveSensing(void){
    pinMode(CGENERATE_GPIO, OUTPUT);
    pinMode(CSENS0_GPIO, INPUT);
    pinMode(CSENS1_GPIO, INPUT);
}

void runCapcitiveSensing(long* sensVal1, long* sensVal2){
    
    digitalWrite(CGENERATE_GPIO, LOW);
    delay(10);
    long startTime = micros();
    digitalWrite(CGENERATE_GPIO, HIGH);
    while(digitalRead(CSENS0_GPIO) == LOW);
    *sensVal1 = micros() - startTime;
    
    digitalWrite(CGENERATE_GPIO, LOW);
    delay(10);
    startTime = micros();
    digitalWrite(CGENERATE_GPIO, HIGH);
    while(digitalRead(CSENS1_GPIO) == LOW);
    *sensVal2 = micros() - startTime;

   // Serial.print("CapacitiveSensing1: ");
   // Serial.print(*sensVal1);
   // Serial.print(" CapacitiveSensing2: ");
    //Serial.print(*sensVal2);
    //Serial.print("\n");
}

