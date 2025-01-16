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

#include <NimBLEDevice.h>

BLEServer* pServer = NULL;
//BLECharacteristic* pCharacteristic = NULL; // For testing
BLECharacteristic* pCharacteristicLED = NULL;
BLECharacteristic* pCharacteristicBatVolt = NULL;
BLECharacteristic* pCharacteristicCapSense = NULL;
BLECharacteristic* pCharacteristicAcc = NULL;



bool deviceConnected = false;
bool oldDeviceConnected = false;
//uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

// #define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // For testing
// #define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" // For testing


#define SERVICE_UUID_LED            "2405915c-3f36-4ff8-9984-f483f4e1469d"
#define CHARACTERISTIC_UUID_LED     "4e99a47c-780d-41be-83f3-925155d8944a"

#define SERVICE_UUID_SENSORS            "924dfb6b-c57f-4187-b796-bde5bf3e7819"
#define CHARACTERISTIC_UUID_BATVOL  "b66780c6-4a43-4ed6-85b2-535e244e1375"
#define CHARACTERISTIC_UUID_CAPSENSE  "fcc60129-6a4c-4600-9588-2f1d35b925d7"
#define CHARACTERISTIC_UUID_ACC  "269bdc96-e17a-4d14-805f-d5bfeb6504fb"


/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, BLEConnInfo& connInfo) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer, BLEConnInfo& connInfo, int reason) {
      deviceConnected = false;
    }
};

// Stacks 3 floats together
void threeFloatsToByteArray(float float1, float float2, float float3, byte* byteArray) {
    // Copy each float into the byte array
    memcpy(byteArray, &float1, sizeof(float));
    memcpy(byteArray + sizeof(float), &float2, sizeof(float));
    memcpy(byteArray + 2 * sizeof(float), &float3, sizeof(float));
}

// Stacks 3 floats together
void twoLongToByteArray(long long1, long long2, byte* byteArray) {
    // Copy each float into the byte array
    memcpy(byteArray, &long1, sizeof(long));
    memcpy(byteArray + sizeof(long), &long2, sizeof(long));
}

bool updateBLE(uint32_t value, float batVol, long capSense0, long capSense1, float x_g, float y_g, float z_g){
        // notify changed value
        if(deviceConnected) {
            //Test Live Characteristic
            // pCharacteristic->setValue((uint8_t*)&value, 4); // For testing
            // pCharacteristic->notify(); // Sends notification to subscribed devices that new infomation is available


            // CapcitiveSensing
            // Byte array to hold 3 floats (3 * 4 bytes)
            byte byteArrayCap[2 * sizeof(long)];
            // Convert the floats to a byte array
            twoLongToByteArray(capSense0, capSense1, byteArrayCap);
            pCharacteristicCapSense->setValue((uint8_t*)&byteArrayCap, 2 * sizeof(long));
            pCharacteristicCapSense->notify(); // Sends notification to subscribed devices that new infomation is available


            // LED
            //digitalWrite(LED_GPIO,pCharacteristicLED->getValue<uint8_t>()); 

            // BattVoltage
            pCharacteristicBatVolt->setValue((uint8_t*)&batVol, 4);

            // Accelerometer
            // Byte array to hold 3 floats (3 * 4 bytes)
            byte byteArrayAcc[3 * sizeof(float)];
            // Convert the floats to a byte array
            threeFloatsToByteArray(x_g, y_g, z_g, byteArrayAcc);

            pCharacteristicAcc->setValue((uint8_t*)&byteArrayAcc, 3 * sizeof(float));
            pCharacteristicAcc->notify(); // Sends notification to subscribed devices that new infomation is available

            

            //vTaskDelay(100/portTICK_PERIOD_MS);  // bluetooth stack will go into congestion, if too many packets are sent
        }

        // disconnecting
        if(!deviceConnected && oldDeviceConnected) {
            vTaskDelay(500/portTICK_PERIOD_MS); // give the bluetooth stack the chance to get things ready
            pServer->startAdvertising(); // restart advertising
            printf("Start advertising\n");
            oldDeviceConnected = deviceConnected;
        }
        // connecting
        if(deviceConnected && !oldDeviceConnected) {
            // do stuff here on connecting
            oldDeviceConnected = deviceConnected;
        }
        return deviceConnected;
}

void setupBLE(void){
    BLEDevice::init("MOUTHIO");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // For test purposes
    // // Create the BLE Service
    // BLEService *pService = pServer->createService(SERVICE_UUID);

    // // Create a BLE Characteristic: TEST
    // pCharacteristic = pService->createCharacteristic(
    //                     CHARACTERISTIC_UUID,
    //             /******* Enum Type NIMBLE_PROPERTY now *******
    //                      BLECharacteristic::PROPERTY_READ   |
    //                     BLECharacteristic::PROPERTY_WRITE  |
    //                     BLECharacteristic::PROPERTY_NOTIFY |
    //                     BLECharacteristic::PROPERTY_INDICATE
    //                 );
    //             **********************************************/
    //                     NIMBLE_PROPERTY::READ   |
    //                     NIMBLE_PROPERTY::WRITE  |
    //                     NIMBLE_PROPERTY::NOTIFY |
    //                     NIMBLE_PROPERTY::INDICATE
    //                 );

      
    // Create the BLE Service: LED STATE
    BLEService *pServiceLED = pServer->createService(SERVICE_UUID_LED);

    // Create a BLE Characteristic
    pCharacteristicLED = pServiceLED->createCharacteristic(
                        CHARACTERISTIC_UUID_LED,
                /******* Enum Type NIMBLE_PROPERTY now *******
                         BLECharacteristic::PROPERTY_READ   |
                        BLECharacteristic::PROPERTY_WRITE  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE
                    );
                **********************************************/
                        NIMBLE_PROPERTY::READ   |
                        NIMBLE_PROPERTY::WRITE 
                    );

    
    bool LED_init = 1;
    pCharacteristicLED->setValue((uint8_t*)&LED_init, 4); 
    // Create a BLE Characteristic


    // Create the BLE Service_ BATTERZ VOLTAGE
    BLEService *pServiceSensors = pServer->createService(SERVICE_UUID_SENSORS);
    
    pCharacteristicBatVolt = pServiceSensors->createCharacteristic(
                        CHARACTERISTIC_UUID_BATVOL,
                /******* Enum Type NIMBLE_PROPERTY now *******
                         BLECharacteristic::PROPERTY_READ   |
                        BLECharacteristic::PROPERTY_WRITE  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE
                    );
                **********************************************/
                        NIMBLE_PROPERTY::READ   
                    );

    // Create a BLE Characteristic_ CAPCAITIVE SENSING 0
    pCharacteristicCapSense = pServiceSensors->createCharacteristic(
                        CHARACTERISTIC_UUID_CAPSENSE,
                /******* Enum Type NIMBLE_PROPERTY now *******
                         BLECharacteristic::PROPERTY_READ   |
                        BLECharacteristic::PROPERTY_WRITE  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE
                    );
                **********************************************/
                        NIMBLE_PROPERTY::NOTIFY |
                        NIMBLE_PROPERTY::READ   
                    );


// Create a BLE Characteristic ACC X
    pCharacteristicAcc = pServiceSensors->createCharacteristic(
                        CHARACTERISTIC_UUID_ACC,
                /******* Enum Type NIMBLE_PROPERTY now *******
                         BLECharacteristic::PROPERTY_READ   |
                        BLECharacteristic::PROPERTY_WRITE  |
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_INDICATE
                    );
                **********************************************/
                        NIMBLE_PROPERTY::NOTIFY |
                        NIMBLE_PROPERTY::READ   
                    );
    
    // Create a BLE Descriptor
    /***************************************************
     NOTE: DO NOT create a 2902 descriptor.
    it will be created automatically if notifications
    or indications are enabled on a characteristic.

    pCharacteristic->addDescriptor(new BLE2902());
    ****************************************************/
    // Start the service
    //pService->start(); // For testing
    pServiceLED->start();
    pServiceSensors->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    //pAdvertising->addServiceUUID(SERVICE_UUID); // For testing
    pAdvertising->addServiceUUID(SERVICE_UUID_LED);
    pAdvertising->addServiceUUID(SERVICE_UUID_SENSORS);

    pAdvertising->setScanResponse(false);
    /** This method had been removed **
     pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
    **/
    BLEDevice::startAdvertising();
    Serial.println("Start advertising BLE...");

}

