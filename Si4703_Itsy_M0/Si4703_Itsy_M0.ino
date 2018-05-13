#include <LiquidCrystal.h>
#include <SparkFunSi4703.h>
#include <Wire.h>

// define the pins for the LCD 
int DB4 = 10;
int DB5 = 11;
int DB6 = 12;
int DB7 = 13;
int E = 9;
int RS = 7;

// pins for radio
int resetPin = 2;
int SDIO = SCL;
int SCLK = SDA;

// vars for radio
int channel;
int volume;
char rdsBuffer[10];
const int UPDATE_INTERVAL_MILLIS = 16000;
bool updatingRDS = false;
uint32_t timer = millis();

// button controls
//int channelDown = 4;
//int channelUp = 3;
//int val = 0;

void updateRDS();
void updateLCD();
void displayInfo();

LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7);
Si4703_Breakout radio(resetPin, SDIO, SCLK);

void setup()
{
  Serial.begin(9600);
  // define pushbutton pins as inputs
//  pinMode(channelDown,INPUT);
//  pinMode(channelUp,INPUT);

  delay(1000); //delay to account for powering up of Arduino and lag to get LCD running
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);

  // display a title 'FM Radio' on line 0 of the LCD, centred
  // (note: line 1 is the second row, since counting begins with 0):  
  lcd.setCursor(0,0);
  lcd.print("    FM Radio    ");
  // show the channel frequency on the LCD, centred
  lcd.setCursor(0,1);
  lcd.print("     Aloha      ");

  radio.powerOn();

  volume = 15;
  radio.setVolume(volume);

  channel = 883; // Jazz
  radio.setChannel(channel);
  
  updateLCD();
}

void loop()
{ 
  if (Serial.available())
  {
    Serial.println("serial available");
    char ch = Serial.read();
    if (ch == 'u') 
    {
      channel = radio.seekUp();
      displayInfo();
    } 
    else if (ch == 'd') 
    {
      channel = radio.seekDown();
      displayInfo();
    } 
    else if (ch == '+') 
    {
      volume ++;
      if (volume == 16) volume = 15;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == '-') 
    {
      volume --;
      if (volume < 0) volume = 0;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == 'a')
    {
      channel = 883; // Jazz
      radio.setChannel(channel);
      displayInfo();
    }
    else if (ch == 'b')
    {
      channel = 974; // BBC R4
      radio.setChannel(channel);
      displayInfo();
    }
    else if (ch == 'r')
    {
      updateRDS();
    }
  }

  if (timer > millis())  timer = millis();

  if (millis() - timer > UPDATE_INTERVAL_MILLIS) {
    Serial.println("Time to update RDS");
    timer = millis();
    updateRDS();
  }
}

void updateRDS()
{
  if (updatingRDS == false) {
    updatingRDS = true;
    Serial.println("RDS listening");
    radio.readRDS(rdsBuffer, 15000);
    Serial.print("RDS heard:");
    Serial.println(rdsBuffer);
    updateLCD();
  }
}

void updateLCD()
{ 
  //convert the integer channel into a string to add decimal place and "MHz" before displaying on LCD
  String channelString = String(channel); 
  String frequency;
  if (channelString.length() == 3) //if the frequency is 3 digits, place a decimal point after the second digit
  {
      frequency = channelString.substring(0,2) + "." + channelString.substring(2) + " MHz";
  }
  else if (channelString.length() == 4) //if the frequency is 4 digits, place a decimal point after the third digit
  {
      frequency = channelString.substring(0,3) + "." + channelString.substring(3) + " MHz";
  }
  
  //first clear line 1 of the display to account for transition between 3 and 4 digit frequencies
  lcd.setCursor(0,1);
  lcd.print("                ");
  //update the frequency on the LCD
  lcd.setCursor(4,1);
  lcd.print(frequency);
  
  // rds info on first line
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(rdsBuffer);
}

void displayInfo()
{
   Serial.print("Channel:"); Serial.print(channel);
   Serial.print(" Volume:"); Serial.println(volume);
   updateLCD();
}
