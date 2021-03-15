const int motor= 12; //LED connected to digital pin 12

void setup()
{
pinMode(motor, OUTPUT); //sets the digital pin as output
}

void loop()
{
digitalWrite(motor,HIGH); //turns the LED on
//delay(5000);
//digitalWrite(motor,LOW);
//delay(1000);
}
