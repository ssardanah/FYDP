# include <SPI.h>

// SPI Defines
#define FrmRdyInt 9
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
// Set threshold
// Get threshold
