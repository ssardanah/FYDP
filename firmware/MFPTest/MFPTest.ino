#include <SPI.h>
#include "mbed.h"

#define IR_LED             5
#define POT_INCREMENT      1
#define POT_INITIAL        0xA5 //100 or 0x64
#define POT_ADDRESS        0x00
#define CS_POT             4 
#define CLK_SPEED          4000000

int potValue = POT_INITIAL;
int newPotValue;  

mbed::DigitalInOut SET_IR_LED(digitalPinToPinName(IR_LED));
mbed::DigitalInOut SET_CS_POT(digitalPinToPinName(CS_POT));

void setup() {
  SET_CS_POT.output();
  SET_IR_LED.output();
  
  //SET_IR_LED = HIGH; // turn LEDs on
  delay(500);
  
  setPotValue(POT_INITIAL); 

}

void loop() {
  // put your main code here, to run repeatedly:

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
