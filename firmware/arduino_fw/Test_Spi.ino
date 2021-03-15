#include <SPI.h>
#include "mbed.h"

#define CLK_SPEED 1000000
#define CS_SENSOR 4
#define FrmRdyInt 9

mbed::DigitalInOut csPin( digitalPinToPinName( CS_SENSOR ) );
mbed::DigitalInOut frPin( digitalPinToPinName( FrmRdyInt ) );

#define TX_LEN    157

void setup() {
  // put your setup code here, to run once:
  csPin.output();
  frPin.input();
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Start setup");

  uint8_t test = 0xFF;

  SPI.begin();
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3));
  csPin = LOW;
  Serial.println("Sent CR");
  test = SPI.transfer(0xF0);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  Serial.print("Received : ");
  Serial.println(test);
  
  csPin = HIGH;
  SPI.endTransaction();
  delay(1000);
  
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3));
  csPin = LOW;
  Serial.println("Sent Set Thresholds");
  test = SPI.transfer(0b11001100);
  SPI.transfer(0xB3);
  SPI.transfer(0x00);
  Serial.print("Received : ");
  Serial.println(test);
  
  csPin = HIGH;
  SPI.endTransaction();

  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3));
  csPin = LOW;
  Serial.println("Sent Wake Up");
  test = SPI.transfer(0b11000011);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  Serial.print("Received : ");
  Serial.println(test);
  
  csPin = HIGH;
  SPI.endTransaction();

  
  
  Serial.println("Leave setup");
}

void loop() {

  
  uint8_t test = 0xFF;
  // put your main code here, to run repeatedly:
  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3));
  csPin = LOW;
  Serial.println("Sent SI");
  test = SPI.transfer(0b10111000);
  SPI.transfer(0b01001110);
  SPI.transfer(0b01001000);
  Serial.print("Received : ");
  Serial.println(test);
  
  csPin = HIGH;
  SPI.endTransaction();

  while(!frPin);

  SPI.beginTransaction(SPISettings(CLK_SPEED, MSBFIRST, SPI_MODE3));
  csPin = LOW;
  Serial.println("Sent R08");
  test = SPI.transfer(0b10011001);
  SPI.transfer(0x02);
  SPI.transfer(0x8F);
  Serial.print("Received : ");
  Serial.println(test);
  
  uint8_t tx_buffer[TX_LEN] = {0x00};
  SPI.transfer(tx_buffer, TX_LEN);

  SPI.endTransaction();
  csPin = HIGH;

  for(uint16_t i = 0; i < TX_LEN; i++)
  {
    Serial.println(tx_buffer[i]);
  }

  delay(500);
  
}
