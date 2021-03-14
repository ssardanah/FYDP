#include <SPI.h>
#include <ArduinoBLE.h>
#include "mbed.h"

// BLE defines
#define peripheralName    "HaemoLuminate"

//System Defines
#define SYS_MODE  1 // 1 for sensing 0 for testing
                    // Specify test command on line 132

//Detection algorithm defines
#define NUM_PIXELS                  142
#define ATTENUATION_THRESH          0.75
#define BLE_ACTIVE                  0
#define PRESENCE_DETECTION_ACTIVE   0
#define SIZE_DETECTION_ACTIVE       0
#define PIXEL_HEIGHT                100 // micrometers
#define PIXEL_PITCH                 50  // micrometers

// SPI Defines
#define FRAME_READY        9
#define CS_SENSOR          4
#define CS_POT             6
#define IR_LED             5
#define DATA_STATUS_LED    7
#define CLK_SPEED          4000000
// Sensor Defines
#define MLX75306_SPI_FRAME_SIZE 24

#define MLX75306_NOP  0b0000000
#define MLX75306_CR   0b11110000
#define MLX75306_RT   0b11011000
#define MLX75306_WT   0b11001100
#define MLX75306_SI   0b10111000
#define MLX75306_SIL  0b10110100
#define MLX75306_RO1  0b10011100
#define MLX75306_RO2  0b10010110
#define MLX75306_RO4  0b10010011
#define MLX75306_RO8  0b10011001
#define MLX75306_TZ1  0b11101000
#define MLX75306_TZ2  0b11100100
#define MLX75306_TZ12 0b11100010
#define MLX75306_TZ0  0b11100001
#define MLX75306_SM   0b11000110
#define MLX75306_WU   0b11000011

#define START_PIXEL   0x02
#define END_PIXEL     0x8F

//From datasheet N = (ending pixel - starting pixel) + 1 + 17
#define TX_LEN        159
#define NUM_PIXELS    144

// Integration time 16 bits
#define TIME_INT_MSB  0b01001110
#define TIME_INT_LSB  0b01001000

// Threshold values (default HI = 0x0B Lo = 0x03)
#define THRESH_HIGH   0x0B
#define THRESH_LOW    0x03

// Hi-Lo ranges for each zebra test (in LSB)
// Conversion to volts = (System Voltage * ADC_Reading)/Resolution
//                     = (5.0 * ADC_Reading)/2^8
#define TZ1LO_MIN         0     // 0 V     
#define TZ1LO_MAX         40    // 0.78 V
#define TZ1HI_MIN         140   // 2.73 V
#define TZ1HI_MAX         240   // 4.69 V

#define TZ2LO_MIN         0  
#define TZ2LO_MAX         40 
#define TZ2HI_MIN         140
#define TZ2HI_MAX         240

#define TZ12HI_MIN        140
#define TZ12HI_MAX        240

// Potentiometer global variables
#define POT_INCREMENT   1
#define POT_INITIAL     0xFF
#define POT_ADDRESS     0x00
int potValue = POT_INITIAL;
int newPotValue;  
bool dataNeedsAdjustement; 


// BLE Variables 
bool presence = false; 
byte sizeVessel = 0.0; 
bool newPresence; 
byte newSizeVessel;
 
#ifdef BLE_ACTIVE == 1
BLEService bloodVesselDetectionService("180C");  // User defined service
BLEBooleanCharacteristic presenceCharacteristic("2A56", BLERead | BLENotify); // standard 16-bit characteristic UUIDm clients will only be able to read an be notified of an update this
BLEByteCharacteristic sizeCharacteristic("2A57", BLERead | BLENotify);
#endif

//IO defines for increased speed
mbed::DigitalInOut SET_CS_SENSOR(digitalPinToPinName(CS_SENSOR));
mbed::DigitalInOut READ_FRAME_READY(digitalPinToPinName(FRAME_READY));
mbed::DigitalInOut SET_CS_POT(digitalPinToPinName(CS_POT));
mbed::DigitalInOut SET_BUILTIN_LED(digitalPinToPinName(LED_BUILTIN));
mbed::DigitalInOut SET_DATA_STATUS_LED(digitalPinToPinName(DATA_STATUS_LED));
mbed::DigitalInOut SET_IR_LED(digitalPinToPinName(IR_LED));

void setup() 
{
  Serial.begin(9600);
  while(!Serial);

  Serial.println("Start Setup");

  SET_CS_SENSOR.output();
  SET_CS_POT.output();
  SET_IR_LED.output();
  SET_BUILTIN_LED.output();
  READ_FRAME_READY.input();
  SET_DATA_STATUS_LED.output();
  
  if (BLE_ACTIVE == 1)
  {
    if (!BLE.begin()) {   // initialize BLE
      Serial.println("starting BLE failed!");
      while (1);
    }
    
    BLE.setLocalName(peripheralName);  // Set name for connection
    
    BLE.setAdvertisedService(bloodVesselDetectionService); // Advertise service
    
    bloodVesselDetectionService.addCharacteristic(presenceCharacteristic); // Add 1st characteristic to service
    bloodVesselDetectionService.addCharacteristic(sizeCharacteristic); // Add 2nd characteristic to service
    BLE.addService(bloodVesselDetectionService); // Add service
    
    presenceCharacteristic.setValue(presence); // Set presence bool
    sizeCharacteristic.setValue(sizeVessel); // Set vessel size double
  
    BLE.advertise();  // Start advertising
    Serial.print("Peripheral device MAC: ");
    Serial.println(BLE.address());
    Serial.println("Waiting for connections...");
  }
  dataNeedsAdjustement = true; 
 
  // Setup SPI protocol
  SPI.begin();
  // SPI Settings: 
  // Max speed is 20MHz , used 14MHz
  // Clock is idle high (CPOL = 1). Data sampled at rising edge & shifted at falling edge (CPHA = 1).
  // Therefore SPI mode = 3
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 

  // Initialize sensor and sleep
  SET_CS_SENSOR = LOW;
  SPI.transfer(MLX75306_CR);
  SPI.transfer(MLX75306_NOP);
  SPI.transfer(MLX75306_NOP);
  SET_CS_SENSOR = HIGH;
  SPI.endTransaction();
  
  // Give sensor time to setup
  delay(2000);
  
  // Start detection
  set_thresholds(THRESH_LOW,THRESH_HIGH);
  start();  
}

void loop() 
{
  SET_IR_LED = HIGH; // turn LEDs on
  
  // Allocate memory for data
  uint8_t* sensorOutput = (uint8_t*)malloc(NUM_PIXELS); // 8-bit ADC output, length excludes junk data
  uint8_t* sensorOutputAdd = sensorOutput;
  
  if (SYS_MODE == 1)
  {
    set_acquire_8b(sensorOutputAdd);
//    if (dataNeedsAdjustement)
//    { 
//      set_acquire_8b(sensorOutputAdd);
//      dataNeedsAdjustement = adjustSaturation(sensorOutputAdd); 
//    }
    
    
    for (int i = 1; i < NUM_PIXELS-1; i++)
    {
      Serial.print("Pixel Number: ");
      Serial.print(i);
      Serial.print("| ");
      Serial.print("Raw Intensity: ");
      Serial.println(sensorOutput[i]);
    }

    if (PRESENCE_DETECTION_ACTIVE == 1)
    {
      newPresence = detectPresence(sensorOutputAdd);
      Serial.print("| ");
      Serial.print("Vessel Presence: ");
      Serial.println(newPresence);
    }
      
    if (SIZE_DETECTION_ACTIVE == 1)
    {
      if (newPresence == false) 
      {
        newSizeVessel = 0.0; 
      }
      else
      {
         newSizeVessel = detectSize(sensorOutputAdd);
      }
      Serial.print("| ");
      Serial.print("Vessel Size: ");
      Serial.println(newSizeVessel);
    }

    if (BLE_ACTIVE == 1) 
    {
      BLEDevice central = BLE.central();  // Wait for a BLE central to connect
      // If a central is connected to the peripheral:
      if (central) 
      {
        Serial.print("Connected to central MAC: ");
        
        // Print the central's BlueTooth address:
        Serial.println(central.address());
        
        // Turn on the LED to indicate the connection:
        SET_BUILTIN_LED = HIGH;
         
        while (central.connected())
        {
          if ((newPresence!=presence) || (newSizeVessel!= sizeVessel))
          {
            presenceCharacteristic.setValue(newPresence); // Set presence bool
            sizeCharacteristic.setValue(newSizeVessel); // Set vessel size double
            
            if (dataNeedsAdjustement = false) 
            {
              set_acquire_8b(sensorOutputAdd);
            }
            else
            {
              set_acquire_8b(sensorOutputAdd);
              adjustSaturation(sensorOutputAdd); 
            }
            for (int i = 0; i < NUM_PIXELS; i++)
            {
              Serial.print("Pixel Number: ");
              Serial.print(i);
              Serial.print("| ");
              Serial.print("Raw Intensity: ");
              Serial.println(sensorOutputAdd[i]);
            }
        
            if (PRESENCE_DETECTION_ACTIVE == 1)
              {
                newPresence = detectPresence(sensorOutputAdd);
                Serial.print("| ");
                Serial.print("Vessel Presence: ");
                Serial.println(newPresence);
              }
              
              if (SIZE_DETECTION_ACTIVE == 1)
              {
                if (newPresence == false) 
                {
                  newSizeVessel = 0.0; 
                }
                else
                {
                   newSizeVessel = detectSize(sensorOutputAdd);
                }
                Serial.print("| ");
                Serial.print("Vessel Size: ");
                Serial.println(newSizeVessel);
              }
          } 
        }
        // when the central disconnects, turn off the LED:
        SET_BUILTIN_LED = LOW;
        Serial.print("Disconnected from central MAC: ");
        Serial.println(central.address());
      }
    }
  }
  
  else if (SYS_MODE == 0)
  {
      bool result = zebraTest(MLX75306_TZ0,sensorOutputAdd);
      if (result == 1)
      {
      	Serial.println("Test Passed");
      }
      else if (result == 0)
      {
      	Serial.println("Test Failed");
      }
  }

  else if (SYS_MODE == 2)
  {
    set_acquire_8b(sensorOutputAdd);
    Serial.println("Acquired Values");
  }  
  free(sensorOutput);
}

/** Set thresholds
 * This function set the low and high thresholds
 * @param low is the 4 lower bits for the low threshold, 
 * high is the 4 higher bits for the high thresholds
 * @return 0 if the operation succeed, 1 if not.
 */
int set_thresholds(unsigned int low,unsigned int high){
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 
  SET_CS_SENSOR = LOW;
  unsigned int thresholds = ((high << 4) && 0xF0) || (low && 0x0F);
  SPI.transfer(MLX75306_WT); 
  SPI.transfer(thresholds); 
  SPI.transfer(MLX75306_NOP); 
  SET_CS_SENSOR = HIGH; 
  SPI.endTransaction();
  
//  if (thresholds == get_thresholds()) {
//    return 0;
//  } 
//  else 
//  {
//    return 1;
//  } 
}

/** Get thresholds
 * The thresholds can be read back with the Read Thresholds 
 * (RT) command after a WT command or before a SI command. 
 */
unsigned int get_thresholds(){
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 
  SET_CS_SENSOR = LOW;
  SPI.transfer(MLX75306_WT); 
  unsigned int thresholds = SPI.transfer(0x00);
  SPI.transfer(MLX75306_NOP);
  SET_CS_SENSOR = HIGH;
  SPI.endTransaction();

  return thresholds;
}
/**  Start
 * Wake up sensor and begin operation 
 */
void start(){
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 
  SET_CS_SENSOR = LOW;
  SPI.transfer(MLX75306_WU);
  SPI.transfer(MLX75306_NOP);
  SPI.transfer(MLX75306_NOP);
  SET_CS_SENSOR = HIGH;
  SPI.endTransaction();
}

/**  Sleep
 * Put sensor in low-power sleep mode 
 */
void sensorSleep(){
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 
  SET_CS_SENSOR = LOW;
  SPI.transfer(MLX75306_SM);
  SPI.transfer(MLX75306_NOP);
  SPI.transfer(MLX75306_NOP);
  SET_CS_SENSOR = HIGH;
  SPI.endTransaction();
}

/**  Acquire 8 bits
 * get 8-bit sensor ADC output 
 */
void set_acquire_8b(uint8_t *data){
  //creating buffer of 0s of tx_length
  uint8_t* tx_buffer = (uint8_t*)malloc(NUM_PIXELS + 3);
  
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 
  SET_CS_SENSOR = LOW;
  SPI.transfer(MLX75306_SI);
  SPI.transfer(TIME_INT_MSB);
  SPI.transfer(TIME_INT_LSB);
  SET_CS_SENSOR = HIGH;
  SPI.endTransaction();

  SET_DATA_STATUS_LED = HIGH;
  while(!READ_FRAME_READY);

  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 
  SET_CS_SENSOR = LOW;
  SPI.transfer(MLX75306_RO8); //Sanity Byte is returned here
  SPI.transfer(START_PIXEL);  //Previous Control1 Byte is returned here
  SPI.transfer(END_PIXEL);    //Previous Control2 Byte is returned here

  for(int i = 0; i < 9 ; i++)
  {
    SPI.transfer(MLX75306_NOP);
  }
  
  SPI.transfer((void*)tx_buffer, NUM_PIXELS + 3); 
  
  SPI.endTransaction();
  SET_CS_SENSOR = HIGH;
  // 12 junk data at the begining and 3 at the end
  for (int i = 0; i < NUM_PIXELS; i++)   
  {
    data[i] = tx_buffer[i];    
  } 
  
  SET_DATA_STATUS_LED = LOW;
  free(tx_buffer);

  SET_CS_SENSOR = LOW;
  SPI.transfer(MLX75306_CR);
  SPI.transfer(MLX75306_NOP);
  SPI.transfer(MLX75306_NOP);
  SET_CS_SENSOR = HIGH;
  SPI.endTransaction();
}

/** Zebra Test channel
 * These command can be used as an integrity check: 
 * Zebra Test 1: All odd pixels
 * Zebra Test 2: All even pixels
 * Zebra Test 12: All even and odd pixels return high
 * Zebra Test 0: All even and odd pixels return low
 * To exclude the influence of charge due to integrated photocurrents, 
 * the TZ1, TZ2, TZ12 and TZ0 tests must be performed in dark.
 */
bool zebraTest(uint8_t command, uint8_t *data)
{
  //creating buffer of 0s of tx_length
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 
  uint8_t tx_buffer[TX_LEN] = {0x00}; 

  //Begin SPI
  SET_CS_SENSOR = LOW;
  SPI.transfer(command);
  SPI.transfer(MLX75306_NOP);
  SPI.transfer(MLX75306_NOP);
  
  // The received data is stored in the tx_buffer in-place 
  // (the old data is replaced with the data received).
  //SPI.transfer(tx_buffer, TX_LEN);
  SPI.endTransaction();
  SET_CS_SENSOR = HIGH;

  set_acquire_8b(data);
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 
  SET_CS_SENSOR = LOW;
  // 12 junk data at the begining and 2 at the end
  
  for (int i=0; i < sizeof(data); i++)
    { 
    if(command==MLX75306_TZ1){
      //1, 3, 5, .., 143 will return a
      //high value TZ1High, all even pixels 2, 4, .., 144 will return a low value TZ1Low (TZ1Low must be between TZ1Low_MIN & MAX)
       if(((i % 2) == 0) && (data[i]<TZ1LO_MIN || data[i]>TZ1LO_MAX)){
        return false;
       }
       //if odd values are higher than the low threshold test failed
       else if ((i % 2) && (data[i]> TZ1HI_MAX || data[i]<TZ1HI_MIN)){
        return false;
       }
       else{
        //Safe, do nothing
       }
    }
    else if(command==MLX75306_TZ2){
      //all odd pixels 1, 3, 5, .., 143 will return a low value TZ2Low, all even pixels 2, 4, .., 144 will return a high value TZ2High
       if((i % 2) && (data[i]> TZ2LO_MAX || data[i]< TZ2LO_MIN)){
        return false;
       }
       else if (((i % 2) == 0) && (data[i]<TZ2HI_MIN || data[i]>TZ2HI_MAX)){
        return false;
       }
       else{
        //Safe, do nothing
       }
    }
    else if(command==MLX75306_TZ12){
      if((data[i]<TZ12HI_MIN || data[i]>TZ12HI_MAX)){
        return false;
       }
       else{
        //Safe, do nothing
       }
    }
    else if(command==MLX75306_TZ0){
       if(data[i]> TZ2LO_MAX || data[i]<TZ2LO_MIN){
        return false;
       }
       else{
        //Safe, do nothing
       }
    }
    else{
      Serial.println("Command not recognized");
      return false;
    }
   }
  SPI.endTransaction();
 SET_CS_SENSOR = HIGH;
  return true;
}

bool detectPresence(uint8_t *data)
{
  uint8_t oldMax = -1; 
  uint8_t maxValue = 0; 
  double  numLowPixels = 0.0; 
  
  for (int i = 0; i < (TX_LEN - 12 - 2); i++)
  {
    maxValue = max(oldMax, data[i]);
    oldMax = data [i]; 
  } 

  for (int i = 0; i < (TX_LEN - 12 - 2); i++)
  {
    if (data [i] < maxValue * ATTENUATION_THRESH) numLowPixels ++;
  } 
  if (numLowPixels >= NUM_PIXELS/4) return true;
  else return false; 
}

double detectSize(uint8_t *data)
{
  uint8_t oldMax = -1; 
  uint8_t oldMin = 257; 
  uint8_t maxValue = 0; 
  uint8_t minValue = 0; 
  double  numLowPixels = 0.0; 
  double  pixelSize = 0.0; 
  
  for (int i = 0; i < (TX_LEN - 12 - 2); i++)
  {
    maxValue = max(oldMax, data[i]);
    oldMax = data [i]; 
    
    minValue = min(oldMin, data[i]);
    oldMin = data [i]; 
  } 

  for (int i = 0; i < (TX_LEN - 12 - 2); i++)
  {
    if (data [i] < maxValue && data [i] > minValue) 
    {
      numLowPixels ++;
    }
  } 

  pixelSize = numLowPixels * PIXEL_HEIGHT + (numLowPixels - 1) * PIXEL_PITCH;

  return pixelSize;
}

bool adjustSaturation(uint8_t *data)
{ 
  
  int numOverSaturatedPixels = 0; 
  uint8_t maxValue = 0; 
  uint8_t first = 0; 
  uint8_t second = 0;  
  uint8_t third = 0;
  
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    if (data[i] > 250 && data[i] <= 255) numOverSaturatedPixels++; 

    // find the averaged max of the top three points
    if (data[i] > first) {
        third = second;
        second = first;
        first = data[i];
    }
    else if (data[i] > second) {
        third = second;
        second = data[i];
    }

    else if (data[i] > third)
        third = data[i];
  } 

  maxValue = (first + second + third)/3;
  
  // if oversaturated
  if (numOverSaturatedPixels > 0) 
  {
    newPotValue = potValue - POT_INCREMENT;  // decrease voltage
    setPotValue(newPotValue);
    potValue = newPotValue; 
    dataNeedsAdjustement = true; 
  }
  
  // if undersaturated
  if (maxValue < 245) 
  {
    newPotValue = potValue + POT_INCREMENT; // increase voltage 
    setPotValue(newPotValue);
    potValue = newPotValue;
    dataNeedsAdjustement = true; 
  }

  // if in [245, 250] range
  if (maxValue <= 245 && maxValue >= 250) 
  {
    dataNeedsAdjustement = false; 
  }
  return dataNeedsAdjustement; 
}

void setPotValue (byte hexResistance)
{
  byte resistanceByte = hexResistance; 

  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE1)); 
  SET_CS_POT = LOW;
  SPI.transfer(POT_ADDRESS);
  SPI.transfer(resistanceByte);
  SET_CS_POT = HIGH;
  SPI.endTransaction();
}
