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
  writeData(MLX75306_CR);
  writeData(MLX75306_NOP);
  writeData(MLX75306_NOP);
  
  // Give sensor time to setup
  delay(100);

}

void loop() 
{
  unsigned char *myData;
  set_acquire_8b(myData);
}
// To Do: 
// Test function (zebra)
// Set integration time function (affects intensity calculation)

/** Set thresholds
 * This function set the low and high thresholds
 * @param low is the 4 lower bits for the low threshold, 
 * high is the 4 higher bits for the high thresholds
 * @return 0 if the operation succeed, 1 if not.
 */
int set_thresholds(unsigned int low,unsigned int high){
  digitalWrite(CS, LOW); //_cs = 0;
  unsigned int thresholds = ((high << 4) && 0xF0) || (low && 0x0F);
  SPI.transfer(MLX75306_WT); //_spi.write(MLX75306_WT);
  SPI.transfer(thresholds); //_spi.write(thresholds);
  SPI.transfer(0x0); //_spi.write(0x0);
  digitalWrite(CS, HIGH); //_cs = 1;
  
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
  SPI.transfer(MLX75306_WT); //_spi.write(MLX75306_WT);
  unsigned int thresholds = SPI.transfer(0x00);
  SPI.transfer(0x00);
  digitalWrite(CS, HIGH);

  return thresholds;
}
/**  Start
 * Wake up sensor and begin operation 
 */
void start(){
  digitalWrite(CS, LOW);
  SPI.transfer(MLX75306_WU);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  digitalWrite(CS, HIGH);
}

/**  Sleep
 * Put sensor in low-power sleep mode 
 */
void sleep(){
  digitalWrite(CS, LOW);
  SPI.transfer(MLX75306_SM);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  digitalWrite(CS, HIGH);
}

/**  Acquire 8 bits
 * get 8-bit sensor ADC output 
 */
void set_acquire_8b(char *data){
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

  //From datasheet N = (ending pixel - starting pixel) + 1 + 17
  int tx_length = (143 - 2) + 1 + 17;
  char tx_buffer[tx_length] = {0x00}; //creating buffer of 0s of tx_length
  SPI.transfer(tx_buffer, tx_length);

  // 10 junk data at the begining //???????????????????????????????????????????????? this might be wrong ????????????????????????????????????????
  // and 4 at the end
  data = (uint8_t*)strncpy(data, &(tx_buffer[10]), 142);
  digitalWrite(CS, HIGH);
}

/**  Write data
 * General write function @param is 1 byte of data to be written
 */
void writeData (byte data)
{
  digitalWrite(CS, LOW);
  SPI.transfer(data);
  digitalWrite(CS, HIGH);
}
