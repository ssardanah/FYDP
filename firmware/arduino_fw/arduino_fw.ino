# include <SPI.h>

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

#define TIME_INT_MSB  0b01001110
#define TIME_INT_LSB  0b01001000

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
  uint8_t *myData = malloc(TX_LEN - 12 - 2); 
  set_acquire_8b(myData);
  
  // To do:
  // -Processing with the data
  // -Print on serial monitor
  
  free(myData);
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
void zebraTest(uint8_t command, uint8_t *data)
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

  // Can print in loop to see output or can add if statements to check bytes
  
  digitalWrite(CS, HIGH);
}
