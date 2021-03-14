#include <SPI.h>

// Potentiometer global variables
#define POT_INCREMENT   1
#define POT_INITIAL     128
#define CS_POT          6
#define POT_ADDRESS     0x00

int potValue = 0;
int newPotValue;  
bool dataNeedsAdjustement; 


void setup() {
  
  SPI.begin();
  pinMode(CS_POT, OUTPUT);
  
  potValue = POT_INITIAL;
  setPotValue(potValue);
  newPotValue = potValue + POT_INCREMENT; 

}

void loop() {
  delay (1000);
  setPotValue(0x55);
  //potValue = newPotValue; 
  //newPotValue = potValue + POT_INCREMENT;
}

void setPotValue (byte hexResistance)
{
  byte resistanceByte = hexResistance; 

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE1)); 
  digitalWrite(CS_POT, LOW);
  SPI.transfer(POT_ADDRESS);
  SPI.transfer(resistanceByte);
  digitalWrite(CS_POT, HIGH);
  SPI.endTransaction();
}
