# include <SPI.h>

//System Defines
#define SYS_VOLT  5.0 
#define SYS_RES   (2^8)

// SPI Defines
#define FrmRdyInt 9
#define CS        10
#define MOSI      11
#define MISO      12
#define SCK       13

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
#define TX_LEN        (143 - 2 + 1 + 17)

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


void setup() 
{
  // Setup serial monitor & SPI protocol
  Serial.begin(9600);
  
  // SPI Settings: 
  // Max speed is 20MHz , used 14MHz
  // Clock is idle high (CPOL = 1). Data sampled at rising edge & shifted at falling edge (CPHA = 1).
  // Therefore SPI mode = 3 
  
  SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE3)); 
  
  pinMode(FrmRdyInt, INPUT);
  pinMode(CS, OUTPUT);
  pinMode(13, OUTPUT); //onboard LED for debugging

  // Initialize sensor and sleep
  digitalWrite(CS, LOW);
  SPI.transfer(MLX75306_CR);
  SPI.transfer(MLX75306_NOP);
  SPI.transfer(MLX75306_NOP);
  digitalWrite(CS, HIGH);

  // Start detection
  start(); 
  
  // Give sensor time to setup
  delay(100);

}

void loop() 
{
  // Allocate memory for data
  uint8_t *sensorOutput = malloc(TX_LEN - 12 - 2); // 8-bit ADC output, length excludes junk data
  set_acquire_8b(sensorOutput);

  for (int i = 0; i <= (TX_LEN - 12 - 2); i++)
  {
    // Convert to volts (check notation) 
    *sensorOutput = SYS_VOLT * (*(sensorOutput++)) / SYS_RES; 

    //Format and display as a double
    Serial.print("Pixel Number: ");
    Serial.print(i);
    Serial.print("\t");
    Serial.print("Intensity: ");
    Serial.println(sensorOutput[i]);
  }
  // Free allocated memory for data
  free(sensorOutput);
}

/** Set thresholds
 * This function set the low and high thresholds
 * @param low is the 4 lower bits for the low threshold, 
 * high is the 4 higher bits for the high thresholds
 * @return 0 if the operation succeed, 1 if not.
 */
int set_thresholds(unsigned int low,unsigned int high){
  digitalWrite(CS, LOW);
  unsigned int thresholds = ((high << 4) && 0xF0) || (low && 0x0F);
  SPI.transfer(MLX75306_WT); 
  SPI.transfer(thresholds); 
  SPI.transfer(MLX75306_NOP); 
  digitalWrite(CS, HIGH); 
  
  if (thresholds == get_thresholds()) {
    return 0;
  } else {
    return 1;
  } 
}

/** Get thresholds
 * The thresholds can be read back with the Read Thresholds 
 * (RT) command after a WT command or before a SI command. 
 */
unsigned int get_thresholds(){
  digitalWrite(CS, LOW);
  SPI.transfer(MLX75306_WT); 
  unsigned int thresholds = SPI.transfer(0x00);
  SPI.transfer(MLX75306_NOP);
  digitalWrite(CS, HIGH);

  return thresholds;
}
/**  Start
 * Wake up sensor and begin operation 
 */
void start(){
  digitalWrite(CS, LOW);
  SPI.transfer(MLX75306_WU);
  SPI.transfer(MLX75306_NOP);
  SPI.transfer(MLX75306_NOP);
  digitalWrite(CS, HIGH);
}

/**  Sleep
 * Put sensor in low-power sleep mode 
 */
void sleep(){
  digitalWrite(CS, LOW);
  SPI.transfer(MLX75306_SM);
  SPI.transfer(MLX75306_NOP);
  SPI.transfer(MLX75306_NOP);
  digitalWrite(CS, HIGH);
}

/**  Acquire 8 bits
 * get 8-bit sensor ADC output 
 */
void set_acquire_8b(uint8_t *data){
  digitalWrite(CS, LOW);
  SPI.transfer(MLX75306_SI);
  SPI.transfer(TIME_INT_MSB);
  SPI.transfer(TIME_INT_LSB);
  digitalWrite(CS, HIGH);

  digitalWrite(13, LOW);
  //DigitalOut led(LED1);
  while(!digitalRead(FrmRdyInt)) {
     digitalWrite(13, HIGH);
  }
  digitalWrite(13, LOW);
  
  digitalWrite(CS, LOW);
  SPI.transfer(MLX75306_RO8);
  SPI.transfer(START_PIXEL);
  SPI.transfer(END_PIXEL);
  
  //creating buffer of 0s of tx_length
  uint8_t tx_buffer[TX_LEN] = {0x00}; 
  // The received data is stored in the tx_buffer in-place 
  // (the old data is replaced with the data received).
  SPI.transfer(tx_buffer, TX_LEN);

  // 12 junk data at the begining and 2 at the end
  for (int i = 12; i < (TX_LEN - 2); i++)
  {
    data[i - 12] = tx_buffer[i];    
  } 
  
  digitalWrite(CS, HIGH);
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
  uint8_t tx_buffer[TX_LEN] = {0x00}; 
  tx_buffer[0] = command;
  tx_buffer[1] = MLX75306_NOP;
  tx_buffer[2] = MLX75306_NOP;

  //Begin SPI
  digitalWrite(CS, LOW);
  
  // The received data is stored in the tx_buffer in-place 
  // (the old data is replaced with the data received).
  SPI.transfer(tx_buffer, TX_LEN);

  // 12 junk data at the begining and 2 at the end
  for (int i = 12; i < (TX_LEN - 2); i++)
  {
    data[i - 12] = tx_buffer[i];    
  } 

  // To do: If staments to check if pixel conditions described in each zebra test in the datasheets hold
  //        Use ranges for high and low in defines
 
  bool test_status = true;
  for (int i=0; i < sizeof(data); i++)
    { 
    if(command==MLX75306_TZ1){
      //1, 3, 5, .., 143 will return a
      //high value TZ1High, all even pixels 2, 4, .., 144 will return a low value TZ1Low
       if(((i % 2) == 0) && data[i]<THRESH_HIGH){
        test_status = false;
       }
       //if odd values are higher than the low threshold test failed
       else if ((i % 2) && data[i]>THRESH_LOW){
        test_status = false;
       }
       else{
        //Safe, do nothing
       }
    }
    else if(command==MLX75306_TZ2){
      //all odd pixels 1, 3, 5, .., 143 will return a low value TZ2Low, all even pixels 2, 4, .., 144 will return a high value TZ2High
       if((i % 2) && data[i]<THRESH_HIGH){
        test_status = false;
       }
       else if (((i % 2) == 0) && data[i]>THRESH_LOW){
        test_status = false;
       }
       else{
        //Safe, do nothing
       }
    }
    else if(command==MLX75306_TZ12){
      if(data[i]<THRESH_HIGH){
        test_status = false;
       }
       else{
        //Safe, do nothing
       }
    }
    else if(command==MLX75306_TZ0){
       if(data[i]>THRESH_LOW){
        test_status = false;
       }
       else{
        //Safe, do nothing
       }
    }
    else{
      Serial.println("Command not recognized");
      test_status = false;
    }
   }
   return test_status;
  digitalWrite(CS, HIGH);
}
