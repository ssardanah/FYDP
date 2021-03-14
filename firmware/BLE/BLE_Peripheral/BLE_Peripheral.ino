#include <ArduinoBLE.h>

#define peripheralName    "HaemoLuminate"


bool presence = false; 
bool newPresence;
byte temperature = 0.0; 
byte newTemperature;  

BLEService bloodVesselDetectionService("0000180C-0000-1000-8000-00805F9B34FB");  // User defined service

BLEBooleanCharacteristic presenceCharacteristic("00002866-0000-1000-8000-00805F9B34FB", BLERead); // standard 16-bit characteristic UUIDm clients will only be able to read an be notified of an update this
BLEByteCharacteristic temperatureCharacteristic("00002867-0000-1000-8000-00805F9B34FB", BLERead); // standard 16-bit characteristic UUIDm clients will only be able to read an be notified of an update this

void setup() {
  Serial.begin(9600);    // initialize serial communication
  while (!Serial);

  pinMode(LED_BUILTIN, OUTPUT); // initialize the built-in LED pin

  if (!BLE.begin()) {   // initialize BLE
    Serial.println("starting BLE failed!");
    while (1);
  }

  BLE.setLocalName(peripheralName);  // Set name for connection
  
  BLE.setAdvertisedService(bloodVesselDetectionService); // Advertise service
  
  bloodVesselDetectionService.addCharacteristic(presenceCharacteristic); // Add 1st characteristic to service
  bloodVesselDetectionService.addCharacteristic(temperatureCharacteristic); // Add 1st characteristic to service
  
  BLE.addService(bloodVesselDetectionService); // Add service
  
  presenceCharacteristic.setValue(presence); // Set presence bool
  presenceCharacteristic.setValue(temperature); // Set presence bool

  BLE.advertise();  // Start advertising
  Serial.print("Peripheral device MAC: ");
  Serial.println(BLE.address());
  Serial.println("Waiting for connections...");

  BLE.advertise(); 
  // To do: SPI setup-here
}


void loop() {
  BLEDevice central = BLE.central();  // Wait for a BLE central to connect
  // if a central is connected to the peripheral:
  if (central) {
    Serial.print("Connected to central MAC: ");
    
    // print the central's BlueTooth address:
    Serial.println(central.address());
    
    // turn on the LED to indicate the connection:
    digitalWrite(LED_BUILTIN, HIGH);

     // To do: add sensor firmware/ data extraction / blood vessel detection
     // Update boolean newPresence and double newSizeVessel
     
    while (central.connected()){
      /*
      if ((newPresence!=presence) || (newSizeVessel!= sizeVessel))
      {
        presenceCharacteristic.setValue(newPresence); // Set presence bool
        sizeCharacteristic.setValue(newSizeVessel); // Set vessel size double
      } 
      */
 
    }
    
    // when the central disconnects, turn off the LED:
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Disconnected from central MAC: ");
    Serial.println(central.address());
  }

}

/*
 * 
 */
