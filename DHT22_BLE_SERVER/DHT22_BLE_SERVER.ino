#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "DHT.h"

#define DHTPIN 5
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

#define temperatureCelsius // Comment for Fahrenheit readings

#define bleServerName "DHT22_ESP32S3"

float temp;
float hum;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 1000;

bool deviceConnected = false;

#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"

// Temperature Characteristic and Descriptor
#ifdef temperatureCelsius
  BLECharacteristic TemperatureCelsiusCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor TemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));
#else
  BLECharacteristic TemperatureFahrenheitCharacteristics("f78ebbff-c8b7-4107-93de-889a6a06d408", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor TemperatureFahrenheitDescriptor(BLEUUID((uint16_t)0x2902));
#endif

// Humidity Characteristic and Descriptor
BLECharacteristic HumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor HumidityDescriptor(BLEUUID((uint16_t)0x2903));

// Setup callbacks for connection events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Initialize DHT sensor
  dht.begin();

  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *dhtService = pServer->createService(SERVICE_UUID);

  // Add BLE Characteristics and Descriptors
  #ifdef temperatureCelsius
    dhtService->addCharacteristic(&TemperatureCelsiusCharacteristics);
    TemperatureCelsiusDescriptor.setValue("DHT temperature Celsius");
    TemperatureCelsiusCharacteristics.addDescriptor(&TemperatureCelsiusDescriptor);
  #else
    dhtService->addCharacteristic(&TemperatureFahrenheitCharacteristics);
    TemperatureFahrenheitDescriptor.setValue("DHT temperature Fahrenheit");
    TemperatureFahrenheitCharacteristics.addDescriptor(&TemperatureFahrenheitDescriptor);
  #endif

  dhtService->addCharacteristic(&HumidityCharacteristics);
  HumidityDescriptor.setValue("DHT humidity");
  HumidityCharacteristics.addDescriptor(&HumidityDescriptor);  // Adding the descriptor explicitly
  HumidityCharacteristics.addDescriptor(new BLE2902());

  // Start the service
  dhtService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting for client connection...");
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {
      hum = dht.readHumidity();
      temp = dht.readTemperature();

      if (isnan(hum) || isnan(temp)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
      }

      // Notify temperature
      #ifdef temperatureCelsius
        char temperatureCTemp[8]; 
        dtostrf(temp, 6, 2, temperatureCTemp);
        TemperatureCelsiusCharacteristics.setValue(temperatureCTemp);
        TemperatureCelsiusCharacteristics.notify();
        Serial.print("Temperature Celsius: ");
        Serial.print(temp);
        Serial.println(" ºC");
      #else
        tempF = dht.readTemperature(true);
        char temperatureFTemp[8];  
        dtostrf(tempF, 6, 2, temperatureFTemp);
        TemperatureFahrenheitCharacteristics.setValue(temperatureFTemp);
        TemperatureFahrenheitCharacteristics.notify();
        Serial.print("Temperature Fahrenheit: ");
        Serial.print(tempF);
        Serial.println(" ºF");
      #endif

      // Notify humidity
      char humidityTemp[8];  
      dtostrf(hum, 6, 2, humidityTemp);
      HumidityCharacteristics.setValue(humidityTemp);
      HumidityCharacteristics.notify();
      Serial.print("Humidity: ");
      Serial.print(hum);
      Serial.println(" %");

      lastTime = millis();
    }
  }
}
