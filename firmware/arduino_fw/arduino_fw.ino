#include <SPI.h>

// SPI Defines
#define FrmRdyInt 9 //frameready
#define CS        10
#define MOSI      11
#define MISO      12
#define SCK       13

// Sensor Defines
#define MLX75306_SPI_FRAME_SIZE 24 

#define MLX75306_NOP  B0000000
#define MLX75306_CR   B11110000
#define MLX75306_RT   B11011000
#define MLX75306_WT   B11001100
#define MLX75306_SI   B10111000
#define MLX75306_SIL  B10110100
#define MLX75306_RO1  B10011100
#define MLX75306_RO2  B10010110
#define MLX75306_RO4  B10010011
#define MLX75306_RO8  B10011001
#define MLX75306_TZ1  B11101000
#define MLX75306_TZ2  B11100100
#define MLX75306_TZ12 B11100010
#define MLX75306_TZ0  B11100001
#define MLX75306_SM   B11000110
#define MLX75306_WU   B11000011

#define TIME_INT_MSB  B01001110
#define TIME_INT_LSB  B0010010


void setup() 
{
  // Setup serial monitor & SPI protocol
  Serial.begin(9600);
  SPI.begin();
  // SPI Settings: 
  // Max speed is 20MHz , used 14MHz
  // Clock is idle high (CPOL = 1) , Data sampled at rising edge. shifted at falling edge (CPHA = 1)
  // Therefore SPI mode = 3 
  
  SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE3)); 

  pinMode(FrmRdyInt, INPUT);
  pinMode(CS, OUTPUT);

  //To do: init sensor 

  // Give sensor time to setup
  delay(100);

}

void loop() 
{
  

}

// To Do: 
// Write function
// Read function
// Test function (zebra)
// Sleep function
// Get threshold

/** Set thresholds
 * This function set the low and high thresholds
 * @param low is the 4 lower bits for the low threshold, 
 * high is the 4 higher bits for the high thresholds
 * @return 0 if the operation succeed, 1 if not.
 */
int set_thresholds(unsigned int low,unsigned int high){
    digitalWrite(CS, LOW); //_cs = 0;
    int thresholds = ((high << 4) && 0xF0) || (low && 0x0F);
    SPI.transfer(MLX75306_WT); //_spi.write(MLX75306_WT);
    SPI.transfer(thresholds); //_spi.write(thresholds);
    SPI.transfer(0x0); //_spi.write(0x0);
    digitalWrite(CS, HIGH); //_cs = 1;
    
    if (thresholds == get_thresholds()) {
      return 0;
    } else {
      return 1;
    }

return 
}
}
