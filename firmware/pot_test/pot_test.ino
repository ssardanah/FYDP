#include <SPI.h>

// Potentiometer global variables
#define POT_INCREMENT   1
#define POT_INITIAL     128
#define CS_POT          5
#define POT_ADDRESS     0x00
#define UPPER_BYTE_MASK 0x3F
#define LOWER_BYTE_MASK 0xC0

int potValue = 0;
int newPotValue;  
bool dataNeedsAdjustement; 


void setup() {
  pinMode(CS_POT, OUTPUT);
  
  //potValue = POT_INITIAL;
  //setPotValue(potValue);
  //newPotValue = potValue + POT_INCREMENT; 

}

void loop() {
  /*delay (1000);
  pot.setValue(0, newPotValue);
  potValue = newPotValue; 
  newPotValue = potValue + POT_INCREMENT; */
  
  setPotValue(0x00);
  delay (5000); 
  setPotValue(0xFF); 
  delay (5000); 

}

void setPotValue (byte hexResistance)
{
  byte upperByte = (hexResistance >> 2) & UPPER_BYTE_MASK; 
  byte lowerByte = (hexResistance << 6) & LOWER_BYTE_MASK; 
  
  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE1)); 
  digitalWrite(CS_POT, LOW);
  SPI.transfer(upperByte);
  SPI.transfer(lowerByte);
  digitalWrite(CS_POT, HIGH);
  SPI.endTransaction();
}
