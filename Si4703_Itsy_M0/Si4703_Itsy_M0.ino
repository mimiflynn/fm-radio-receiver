#include <LiquidCrystal.h>
#include <SparkFunSi4703.h>
#include <Wire.h>
#include "ircodes.h"

#define DEBUG true

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
int channelDown = 4;
int downButtonState;
int lastDownButtonState = LOW;
int channelUp = 3;
int upButtonState;
int lastUpButtonState = LOW;
unsigned long lastDownDebounceTime = 0;
unsigned long lastUpDebounceTime = 0;
unsigned long debounceDelay = 50;

// remote
const uint8_t IRpin = A5;
const int MAXPULSE = 65000;
const int NUMPULSES = 50;
const int RESOLUTION = 20;
const int FUZZINESS = 30;
uint16_t pulses[NUMPULSES][2];
uint8_t currentpulse = 0;

void displayInfo();
void listenChannelDownButton();
void listenChannelUpButton();
void listenKeyboard();
void updateRDS();
void updateLCD();

LiquidCrystal lcd(RS, E, DB4, DB5, DB6, DB7);
Si4703_Breakout radio(resetPin, SDIO, SCLK);

void setup()
{
  Serial.begin(9600);
  
  // define pushbutton pins as inputs
  pinMode(channelDown,INPUT);
  pinMode(channelUp,INPUT);

  delay(1000);
  
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

  listenChannelDownButton();
  listenChannelUpButton();
  listenKeyboard();
  listenRemote();
  

//  if (timer > millis())  timer = millis();
//
//  if (millis() - timer > UPDATE_INTERVAL_MILLIS) {
//    Serial.println("Time to update RDS");
//    timer = millis();
//    updateRDS();
//  }
}

void listenRemote() {
  int numberpulses;
  
  numberpulses = listenForIR();
  
  Serial.println("Heard ");
  Serial.print(numberpulses);
  Serial.println("-pulse long IR signal");

  // ignore random pulses from somewhere
  if (numberpulses > 30) {
    if (IRcompare(numberpulses, IRchannelUp, sizeof(IRchannelUp)/4)) {
      Serial.println("channel up");
      channel = radio.seekUp();
      displayInfo();
    }
    if (IRcompare(numberpulses, IRchannelDown, sizeof(IRchannelDown)/4)) {
      Serial.println("channel down");
      channel = radio.seekDown();
      displayInfo();
    }
    if (IRcompare(numberpulses, IRsource, sizeof(IRsource)/4)) {
      Serial.println("source");
      channel = 883;
      radio.setChannel(channel);
      displayInfo();
    }
    if (IRcompare(numberpulses, IRtimeShift, sizeof(IRtimeShift)/4)) {
      Serial.println("time shift");
      channel = 923;
      radio.setChannel(channel);
      displayInfo();
    }
  }
  
  delay(500);
}

void listenChannelDownButton()
{
    // Channel down button listener
  int downButtonReading = digitalRead(channelDown);
  
  if (downButtonReading != lastDownButtonState) {
    lastDownDebounceTime = millis();
  }
  
  if ((millis() - lastDownDebounceTime) > debounceDelay) {
    if (downButtonReading != downButtonState) {
      downButtonState = downButtonReading;
      if (downButtonState == HIGH) {
        channel = radio.seekDown();
        displayInfo();
      }
    }
  }

  lastDownButtonState = downButtonReading;
}

void listenChannelUpButton() {
  // Channel up button listener
  int upButtonReading = digitalRead(channelUp);

  if (upButtonReading != lastUpButtonState) {
    lastUpDebounceTime = millis();
  }
  
  if ((millis() - lastUpDebounceTime) > debounceDelay) {
    if (upButtonReading != upButtonState) {
      upButtonState = upButtonReading;
      if (upButtonState == HIGH) {
        channel = radio.seekUp();
        displayInfo();
      }
    }
  }

  lastUpButtonState = upButtonReading;
}

void listenKeyboard() {
  // keyboard serial commands
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
      channel = 883;
      radio.setChannel(channel);
      displayInfo();
    }
    else if (ch == 'b')
    {
      channel = 931;
      radio.setChannel(channel);
      displayInfo();
    }
    else if (ch == 'r')
    {
      updateRDS();
    }
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
    updatingRDS = false;
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

boolean IRcompare(int numpulses, int Signal[], int refsize) {
  int count = min(numpulses,refsize);
  Serial.print("count set to: ");
  Serial.println(count);
  for (int i=0; i< count-1; i++) {
    int oncode = pulses[i][1] * RESOLUTION / 10;
    int offcode = pulses[i+1][0] * RESOLUTION / 10;
    
#ifdef DEBUG    
    Serial.print(oncode); // the ON signal we heard
    Serial.print(" - ");
    Serial.print(Signal[i*2 + 0]); // the ON signal we want 
#endif   
    
    // check to make sure the error is less than FUZZINESS percent
    if ( abs(oncode - Signal[i*2 + 0]) <= (Signal[i*2 + 0] * FUZZINESS / 100)) {
#ifdef DEBUG
      Serial.print(" (ok)");
#endif
    } else {
#ifdef DEBUG
      Serial.print(" (x)");
#endif
      // we didn't match perfectly, return a false match
      return false;
    }
    
    
#ifdef DEBUG
    Serial.print("  \t"); // tab
    Serial.print(offcode); // the OFF signal we heard
    Serial.print(" - ");
    Serial.print(Signal[i*2 + 1]); // the OFF signal we want 
#endif    
    
    if ( abs(offcode - Signal[i*2 + 1]) <= (Signal[i*2 + 1] * FUZZINESS / 100)) {
#ifdef DEBUG
      Serial.print(" (ok)");
#endif
    } else {
#ifdef DEBUG
      Serial.print(" (x)");
#endif
      // we didn't match perfectly, return a false match
      return false;
    }
    
#ifdef DEBUG
    Serial.println();
#endif
  }
  // Everything matched!
  return true;
}

int listenForIR(void) {
  currentpulse = 0;
  
  while (1) {
    uint16_t highpulse, lowpulse;  // temporary storage timing
    highpulse = lowpulse = 0; // start out with no pulse length
  
  while (digitalRead(IRpin)) { // this is too slow!
       // pin is still HIGH
       // count off another few microseconds
       highpulse++;
       delayMicroseconds(RESOLUTION);

       if (((highpulse >= MAXPULSE) && (currentpulse != 0))|| currentpulse == NUMPULSES) {
         printpulses();
         return currentpulse;
       }
    }
    // we didn't time out so lets stash the reading
    pulses[currentpulse][0] = highpulse;
  
    // same as above
//    while (! (_BV(IRpin))) {
      while (! digitalRead(IRpin)) {
       // pin is still LOW
       lowpulse++;
       delayMicroseconds(RESOLUTION);
        // KGO: Added check for end of receive buffer
        if (((lowpulse >= MAXPULSE)  && (currentpulse != 0))|| currentpulse == NUMPULSES) {
          printpulses();
         return currentpulse;
       }
    }
    pulses[currentpulse][1] = lowpulse;

    // we read one high-low pulse successfully, continue!
    currentpulse++;
  }
}

void printpulses(void) {
  Serial.println("\n\r\n\rReceived: \n\rOFF \tON");
  for (uint8_t i = 0; i < currentpulse; i++) {
    Serial.print(pulses[i][0] * RESOLUTION, DEC);
    Serial.print(" usec, ");
    Serial.print(pulses[i][1] * RESOLUTION, DEC);
    Serial.println(" usec");
  }
  
  // print it in a 'array' format
  Serial.println("int IRsignal[] = {");
  Serial.println("// ON, OFF (in 10's of microseconds)");
  for (uint8_t i = 0; i < currentpulse-1; i++) {
    Serial.print("\t"); // tab
    Serial.print(pulses[i][1] * RESOLUTION / 10, DEC);
    Serial.print(", ");
    Serial.print(pulses[i+1][0] * RESOLUTION / 10, DEC);
    Serial.println(",");
  }
  Serial.print("\t"); // tab
  Serial.print(pulses[currentpulse-1][1] * RESOLUTION / 10, DEC);
  Serial.print(", 0");
  Serial.println("};");
}

