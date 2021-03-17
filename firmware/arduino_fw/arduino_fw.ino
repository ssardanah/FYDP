#include <SPI.h>
#include <ArduinoBLE.h>
#include "mbed.h"

// BLE defines
#define peripheralName    "HaemoLuminate"

//System Defines
#define SYS_MODE  2 // 2 for BLE 1 for sensing 0 for testing
                    // Specify test command on line 132

#define PRESENCE_DETECTION_ACTIVE     1
#define TEMPERATURE_DETECTION_ACTIVE  1
#define PIXEL_HEIGHT                  100 // micrometers
#define PIXEL_PITCH                   50  // micrometers

// SPI Defines
#define FRAME_READY        9
#define CS_SENSOR          6 
#define CS_POT             4 
#define IR_LED             5
#define DATA_STATUS_LED    7
#define CLK_SPEED          4000000

// Sensor Defines
#define MLX75306_SPI_FRAME_SIZE 24

#define MLX75306_NOP  0b00000000
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
#define POT_INITIAL     0x55
#define POT_ADDRESS     0x00
int potValue = POT_INITIAL;
int newPotValue;  
bool dataNeedsAdjustement; 


// BLE Variables 
bool presence = false; 
byte temperature = 0.0; 
bool newPresence; 
byte newTemperature;
 
#ifdef SYS_MODE == 2
BLEService bloodVesselDetectionService("0000180C-0000-1000-8000-00805F9B34FB");  // User defined service
BLEBooleanCharacteristic presenceCharacteristic("00002866-0000-1000-8000-00805F9B34FB", BLERead); // standard 16-bit characteristic UUIDm clients will only be able to read an be notified of an update this
BLEDoubleCharacteristic temperatureCharacteristic("00002867-0000-1000-8000-00805F9B34FB", BLERead); // standard 16-bit characteristic UUIDm clients will only be able to read an be notified of an update this
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

  SET_CS_SENSOR = HIGH;
  delay(1000);
  
  if (SYS_MODE == 2)
  {
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
    temperatureCharacteristic.setValue(temperature); // Set vessel size double
  
    BLE.advertise();  // Start advertising
    Serial.print("Peripheral device MAC: ");
    Serial.println(BLE.address());
    Serial.println("Waiting for connections...");
    BLE.advertise(); 
  }


  potValue = POT_INITIAL;
  dataNeedsAdjustement = true; 
  
  // Setup serial monitor & SPI protocol
  Serial.begin(9600); 
 
  // Setup SPI protocol
  SPI.begin();
  // SPI Settings: 
  // Max speed is 20MHz , used 4MHz
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

  SET_IR_LED = HIGH; // turn LEDs on
}

void loop() 
{  
  // Allocate memory for data
  uint8_t* sensorOutput = (uint8_t*)malloc(NUM_PIXELS); // 8-bit ADC output, length excludes junk data
  uint8_t* sensorOutputAdd = sensorOutput;
  double temperatureOutput = 0; // TO REWORK
  
  if (SYS_MODE == 1)
  {
//    if (dataNeedsAdjustement = false) set_acquire_8b(sensorOutput);
//    else
//    {
//      set_acquire_8b(sensorOutput);
//      adjustSaturation (sensorOutput); 
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
      
    if (TEMPERATURE_DETECTION_ACTIVE == 1)
    {
      temperatureOutput = getTemperatureReading(newTemperature);
      Serial.print("| ");
      Serial.print("Temperature: ");
      Serial.println(temperatureOutput);
    }
  }
  
  if (SYS_MODE == 2) 
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
       
      while (central.connected()){
        if ((newPresence!=presence) || (temperatureOutput!= temperature))
        {
          presenceCharacteristic.setValue(newPresence); // Set presence bool
          temperatureCharacteristic.setValue(temperatureOutput); // Set Temperature byte
          presence = newPresence;
          temperature = temperatureOutput;
        }
          
//        if (dataNeedsAdjustement = false) set_acquire_8b(sensorOutput);
//        else
//        {
//          set_acquire_8b(sensorOutput);
//          adjustSaturation (sensorOutput); 
//        }
        
        for (int i = 0; i <= (TX_LEN-12-2); i++)
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
          
        if (TEMPERATURE_DETECTION_ACTIVE == 1)
        {
          temperatureOutput = getTemperatureReading(newTemperature);
          Serial.print("| ");
          Serial.print("Temperature: ");
          Serial.println(temperatureOutput);
        }      
      } 
    
      // when the central disconnects, turn off the LED:
      SET_BUILTIN_LED = LOW;
      Serial.print("Disconnected from central MAC: ");
      Serial.println(central.address());
    }
  }
  else if (SYS_MODE == 0)
  {
    bool result = zebraTest(MLX75306_TZ0,sensorOutput);
    if (result == 1) 
    {
      Serial.println("Test Passed");
    }
    else if (result == 0) 
    {
      Serial.println("Test Failed");
    }
  }
  
  free(sensorOutputAdd); 
  // read the incoming byte:
  //for printing into text file, to be used with python
  /*byte incomingByte = Serial.read();

  if(incomingByte == '0')
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
  */

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
    uint8_t temp = SPI.transfer(MLX75306_NOP); //0x00
    if(i == 5 ) 
    {
      newTemperature = temp;
    }
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

  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3)); 
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
  int counter = 0; 
  double data2[(NUM_PIXELS - 2)];
 
  for (int i=1; i<(NUM_PIXELS - 1); i++){
       data2[counter] = (double)(data[i]/255.0);
        Serial.print("Pixel Number: ");
        Serial.print(counter);
        Serial.print("| ");
        Serial.print("Raw Intensity: ");
        Serial.println(data2[counter]*255.0);
       counter ++;     
  }

   int dataSize = counter;
   int l = 0;
   int z = 0;
   int a = 0; 
   int pos = 0;
   int neg = 0; 
   int clearDip = 0;
   int highValue = 0; 
   int concaveBVup = 0;
   int concaveBVdown = 0;
   int smoothSize = dataSize/5; // if data size is 127 --> 25 --> ??does this round down?
   int fivePointsSize = smoothSize/5;
   int signChangeCounter = 0;
   int slopeSize = 0; 
   int presence = 0;
   double arrayB[5];
   double arrayD[5]; 
   double dataOut[dataSize];
   double dataOutTwo[dataSize];
   double smooth[smoothSize]; 
   double fivePoints[fivePointsSize]; 
   double finalSignal[smoothSize]; 
   double signalGradient[smoothSize];
   double finalGradient[fivePointsSize];
   bool fullConcave;
   bool partConcave; 
       
 /*removing erroneous values*/
  for(int i = 0; i < dataSize; i ++) {
    if(i == 0 || i == (dataSize - 1)) {
      dataOut[i] = data2[i];
    } else if((((1.10*data2[i-1]) < data2[i]) && (data2[i] > (1.10*data2[i+1]))) || (((0.9*data2[i-1]) > data2[i]) && (data2[i] < (0.9*data2[i+1])))) {
         dataOut[i] = (data2[i-1] + data2[i+1])/(2.00);
      } else {
          dataOut[i] = data2[i];
       }
    }
          
//for (int i=0; i < dataSize; i++){          
// Serial.print("Pixel Number: ");
// Serial.print(i);
// Serial.print("| ");
// Serial.print("dataOut: ");
// Serial.println(dataOut[i]);  
//  }

  /*smooth the signal*/ 
  for(int n = 0; n < (dataSize-4); n=n+5) {  
    arrayB[0] = dataOut[n]; //change back to dataOutTwo if get it working
    arrayB[1] = dataOut[n+1];
    arrayB[2] = dataOut[n+2];
    arrayB[3] = dataOut[n+3];
    arrayB[4] = dataOut[n+4];      
    smooth[l] = getMean(arrayB, 5); // do I need to have a * before arrayB to show im passing in a pointer?
    l++;
  }

     
  /*smoothing it further by removing random dips*/
  for(int i = 0; i < smoothSize; i++) {
    if((i == 0) || (i == (smoothSize - 1))) {
      finalSignal[i] = smooth[i];
    } else if(((smooth[i-1] < smooth[i]) && (smooth[i] > smooth[i+1])) || ((smooth[i-1] > smooth[i]) && (smooth[i] < smooth[i+1]))) {
      finalSignal[i] = (smooth[i-1] + smooth[i+1])/(2.00); 
    } else {
        finalSignal[i] = smooth[i];
      }
  }
 
  /*getting the gradient of finalSignal*/ // TESTED AND WORKS CORRECTLY
  for (int i = 0; i < smoothSize; i++) {
    if(i == 0){
      signalGradient[i] = finalSignal[i+1] - finalSignal[i];
    } else if(i == (smoothSize-1)) {
      signalGradient[i] = finalSignal[smoothSize-1] - finalSignal[smoothSize-2];
    } else {
      signalGradient[i] = 0.5 * (finalSignal[i+1] - finalSignal[i-1]); 
    }
  }
   
  /*removing zeros from gradient so can check if signal change*/
  for (int i = 0; i < smoothSize; i++) {
    if ((signalGradient[i] == 0) && (i == 0 || i == (smoothSize - 1))) {
      signalGradient[i] = signalGradient[i];
    } else if ((signalGradient[i] == 0) && (signalGradient[i-1] <= 0)) {
      signalGradient[i] = -0.0000001;
    } else if ((signalGradient[i] == 0) && (signalGradient[i-1] >= 0)) {
      signalGradient[i] = 0.0000001;
    } else {
      signalGradient[i] = signalGradient[i]; 
    }
  }
   
  /*checking the number of sign changes*/
  for(int i = 0; i < (smoothSize-1); i++) {
    if((signalGradient[i]*signalGradient[i+1]) < 0) {
      signChangeCounter = signChangeCounter + 1;
    }
  }
    
  /*create an array of that size and fill the array with the indeces of sign changes*/
  int signChange[signChangeCounter];
  if (signChangeCounter != 0) {
    for(int i = 0; i < (smoothSize - 1); i++) {
      if((signalGradient[i]*signalGradient[i+1]) < 0) {
        signChange[a] = i, 
        a = a + 1;
      }
    }
  }
  
  /*create a slope array with the slope of each section*/
  slopeSize = (signChangeCounter + 1); 
  double slope[slopeSize]; 

  if(signChangeCounter == 0) {
    slope[0] = (finalSignal[smoothSize - 1] - finalSignal[0])/(smoothSize-1-0); //if no sign change, min and max should be at endpoints
  } else if(signChangeCounter > 0) {
    if(slopeSize == 2) {
      slope[0] = (finalSignal[signChange[0]] - finalSignal[0])/(signChange[0]-0);
      slope[1] = (finalSignal[smoothSize-1] - finalSignal[(signChange[0]+1)])/((smoothSize - 1) - (signChange[0]+1)); 
    } else if(slopeSize > 2) {
      slope[0] = (finalSignal[signChange[0]] - finalSignal[0])/(signChange[0]-0);
      slope[slopeSize-1] = (finalSignal[smoothSize-1] - finalSignal[(signChange[(signChangeCounter - 1)]+1)])/((smoothSize - 1) - (signChange[(signChangeCounter - 1)]+1));
      for(int i = 1; i < (slopeSize - 1); i++) {
        slope[i] = (finalSignal[signChange[i]] - finalSignal[signChange[i-1]+1])/(signChange[i] - (signChange[i-1]+1));
      }
    }
  }

  for(int i = 0; i < slopeSize; i ++) {
    if(slope[i] > 0) { //one positive value 
      pos = pos + 1; 
      if (abs(slope[i]) > 0.0300)
      {
        highValue = highValue + 1; 
      }
    } else if(slope[i] < 0) { //one negative value 
      neg = neg + 1; 
      if  (abs(slope[i]) > 0.0300) {
        highValue = highValue + 1; 
      } 
    }
  }

   //Clear Dip and Concave Detection 
    for(int n = 0; n < (smoothSize-4); n=n+5) {  
      arrayD[0] = finalSignal[n];
      arrayD[1] = finalSignal[n+1];
      arrayD[2] = finalSignal[n+2];
      arrayD[3] = finalSignal[n+3];
      arrayD[4] = finalSignal[n+4];      
      fivePoints[z] = getMean(arrayD, 5); 
      z++;
    } 

  /*getting the gradient of the fivePoints array*/ // TESTED AND WORKS CORRECTLY
  for (int i = 0; i < fivePointsSize; i++) {
    if(i == 0){
      finalGradient[i] = fivePoints[i+1] - fivePoints[i];
    } else if(i == (fivePointsSize-1)) {
      finalGradient[i] = fivePoints[fivePointsSize-1] - fivePoints[fivePointsSize-2];
    } else {
      finalGradient[i] = 0.5 * (fivePoints[i+1] - fivePoints[i-1]); 
    }
  }

    /*check for clear dip*/ 
    for(int i = 0; i < (fivePointsSize - 2); i ++) { 
      if(finalGradient[i] < 0) {
        if((abs(finalGradient[i]) > finalGradient[i+1]) && (abs(finalGradient[i+1]) < finalGradient[i+2])) {
          clearDip = clearDip + 1;
        }
      }
    }
    
    /*check if gradient of five points is ascending --> concave upwards*/
    for(int i = 0; i < (fivePointsSize - 1); i ++) {
      if((finalGradient[i] < finalGradient[i+1]) && ((finalGradient[i] * finalGradient[i+1]) > 0) && (finalGradient[i] > 0)) {
        concaveBVup = concaveBVup + 1;
      } else if((abs(finalGradient[i]) > abs(finalGradient[i+1])) && ((finalGradient[i] * finalGradient[i+1]) > 0) && (finalGradient[i] < 0)) {
        concaveBVdown = concaveBVdown +1;
      }
    }
    

    if((concaveBVup == 4) || (concaveBVdown == 4)) {
      fullConcave = true; 
    } else {
      fullConcave = false; 
    }

    if((concaveBVup == 3) || (concaveBVdown == 3)) {
      partConcave = true; 
    } else {
      partConcave = false; 
    }

  /*DETECTION PORTION*/ 
  if((pos > 0) && (neg > 0) && (highValue > 0)) {
    presence = 1; 
  } else if(clearDip > 0) {
    presence = 2; 
  } else if(fullConcave == true) {
    presence = 3; 
  } else if((slopeSize == 1) && (abs(slope[0]) > 0.015) && (partConcave == true)) {
    presence = 4;  
  } else {
    presence = 0;
  } 

//  Serial.print("Pos: ");
//  Serial.println(pos); 
//  Serial.print("Neg: ");
//  Serial.println(neg); 
//  Serial.print("HighValue: ");
//  Serial.println(highValue);
//  Serial.print("Clear Dip: ");
//  Serial.println(clearDip);  
//  Serial.print("fullConcave: ");
//  Serial.println(fullConcave); 
//  Serial.print("partConcave: ");
//  Serial.println(partConcave);
//  Serial.print("Slope:  ");
//  Serial.println(slope[0]);
//    Serial.print("Presence type: ");
//    Serial.println(presence); 
              
  /*final return*/ 
  if (presence > 0) return true; 
  else return false; 
        
  }


bool adjustSaturation(uint8_t *data)
{ 
  
  int numOverSaturatedPixels = 0; 
  uint8_t maxValue = 0; 
  uint8_t first = 0; 
  uint8_t second = 0;  
  uint8_t third = 0;
  
  for (int i = 1; i < NUM_PIXELS-1; i++)
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

/*
 * Get the mean from an array of ints
 */
double getMean(double * val, int arrayCount) {
  double total = 0;
  for (int i = 0; i < arrayCount; i++) {
    total = total + val[i];
  }
  double avg = (double)(total/arrayCount);
  //Serial.println(avg);
  return avg;
}


double getStdDev(double * val, int arrayCount) {
  double avg = getMean(val, arrayCount);
  double total = 0;
  for (int i = 0; i < arrayCount; i++) {
    total = total + (val[i] - avg) * (val[i] - avg);
  }

  double variance = (double)(total/arrayCount);
  double stdDev = sqrt(variance);
  return stdDev;
}


/*Get gradient function*/
double * getGradient(double * val, int valArraySize) 
{
  double gradient[valArraySize]; //static because we can't return the address of a local variable to outside of the function 
  
  for (int i = 0; i < valArraySize; i++) {
    if(i == 0){
      gradient[i] = val[i+1] - val[i];
    } else if(i == (valArraySize-1)) {
      gradient[i] = val[valArraySize-1] - val[valArraySize-2];
    } else {
      gradient[i] = 0.5 * (val[i+1] - val[i-1]); 
    }
  }
  return gradient;
}

double getTemperatureReading(uint8_t val) 
{
  double slope = (100-75)/(60-80);
  double y_intercept = 165.0;
  double temperature = 0;

  temperature = (val - y_intercept)/slope;
  
  return temperature;
}
    
