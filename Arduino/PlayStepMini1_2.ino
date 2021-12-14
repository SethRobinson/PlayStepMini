
//This code is GPLv3 by Seth A. Robinson

//Some peizo beeper effects are taken or modified from the Volume Library Demo Sketch (c) 2016 Connor Nishijima which is released under
//released under the GPLv3 license.   https://github.com/connornishijima/arduino-volume
//It's marked below where those are

#if defined(__AVR_ATmega328P__)
#define USE_OLED

//#define DEBUG
#endif

#ifdef DEBUG
//Debug mode, use serial output and different pins for things if we want, it's just our Uno testbed
const int LED_PIN = 2//LED_BUILTIN;
#else
const int LED_PIN = 0; //avr88 //0 to disable
#endif

//If you are concerned that #defines would save global memory over const ints, no they won't, the compiler is smarter than that.  I hope

//these are the same for the 88 or 328p
const int SPEAKER_PIN = 3;  //0 to disable, 3 normally
const int PEDAL_PIN = 14; //new units use 14, A0 (old units used 4, but I thought the ADC might be useful someday)
const int HDMI_PIN = 8; //8 (0 to disable) transistor will set it to ground to ruin the hdmi signal
const int HDMI_SECOND_PIN = 9; //9 - 0 to disable.  transistor will set it to ground to ruin the hdmi signal

const int ONE_SECOND_MS = 1000;
const int MAX_ENERGY = 300;
const int WARNING_TIME_SECONDS = 10;
const int ENERGY_EACH_STEP_GIVEN = 4;

const int DEBOUNCE_TIMER_MS = 0; //stop static or whatever from looking like it's stepping fast

#define HDMI_ON LOW
#define HDMI_OFF HIGH
bool bOLEDInitted = false;
bool bOLEDError = false;
#include "pitch.h"

bool bLastPedalHigh = false;
int energy = 1;
unsigned long timerSecondTickMS = 0;
unsigned long timerLastStepMS = 0;

//The "scramble" system basically turns on/off quickly connecting the data0 HDMI pin to ground, to break the signal.  Without this, some TVs are capapable
//of displaying the signal, turns out HDMI is pretty resilient when you don't want it to be
unsigned long scrambleTimerMS = 0;
unsigned long scrambleStepMS = 25;
bool bLastScrambleOn = false;

#define STARTUP_INITIAL 0
#define STARTUP_OLED 1
#define STARTUP_OLED_SETTINGS 2
#define STARTUP_FINISHED 3

int stateStartup = STARTUP_INITIAL;

int stateTimerMS = 0;
int stateTimeBetweenStatesMS = 400;


#ifdef USE_OLED
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  
  // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
  // The pins for I2C are defined by the Wire-library. 
  // On an arduino UNO:       A4(SDA), A5(SCL)
  // On an arduino MEGA 2560: 20(SDA), 21(SCL)
  // On an arduino LEONARDO:   2(SDA),  3(SCL), ...
  #define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  
  #include <Fonts/FreeMonoBold24pt7b.h> //A nice fat font for us.  Could use the default font and just scale it up to save memory though.

#endif

// the setup function runs once when you press reset or power the board
void setup() 
{

   #ifdef DEBUG
    Serial.begin(9600); // open the serial port at 9600 bps:

    Serial.println("Starting up ");
 #endif

  if (LED_PIN != 0)
    pinMode(LED_PIN, OUTPUT);

  if (PEDAL_PIN != 0)
    pinMode(PEDAL_PIN, INPUT);
  
  //prepare hdmi 1
  if (HDMI_PIN != 0)
  {
    pinMode(HDMI_PIN, OUTPUT);
    digitalWrite(HDMI_PIN, HDMI_ON);  //turn it on
  }
  
  //hdmi 2?
  if (HDMI_SECOND_PIN != 0)
  {
    pinMode(HDMI_SECOND_PIN, OUTPUT);
    digitalWrite(HDMI_SECOND_PIN, HDMI_ON);  //turn it on
  }
  
 
  //PlayGetEnergySound();
  //InitOLED();
  timerSecondTickMS = millis();

  //for whatever reason, if I don't stagger out the OLED init stuff, sometimes it fails, so we do that below
  stateTimerMS = millis()+stateTimeBetweenStatesMS;
}

void RunDelayedStartup()
{
   if (stateStartup == STARTUP_INITIAL)
   {
      if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
      {
        #ifdef DEBUG
        Serial.println(F("SSD1306 allocation failed"));
        #endif
        bOLEDError = true;
      } else
      {
       //success
      }
   
     stateStartup = STARTUP_OLED;
     return;
   }

   if (stateStartup == STARTUP_OLED)
   {
    if (!bOLEDError)
    {
      display.dim(true);
      display.clearDisplay();
      display.display();
    }

    stateStartup = STARTUP_OLED_SETTINGS;
    return;
   }

  
  if (stateStartup == STARTUP_OLED_SETTINGS)
  {
    if (!bOLEDError)
    {
          display.setFont(&FreeMonoBold24pt7b);
          bOLEDInitted = true;
    }
 
          stateStartup = STARTUP_FINISHED;
          stateTimerMS = 0;
          return;
  }
     
}

void DrawEnergy()
{
  #ifdef USE_OLED

  if (!bOLEDInitted) return;
  
  display.clearDisplay();
  display.setTextSize(1);             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  String sNum = String(energy);
  int fontX = 30;
  int fontY = 43;

  int offsetX = 0+ (64-(sNum.length()* (fontX/2)));
  display.setCursor(offsetX, fontY);
  display.print(energy, DEC);
  display.display();
  #endif
}

int lastSmoothedPedalRead = LOW; //we have to start with one, afterall
int smoothedPedalRequiredTimeMS = 200;
long smoothedPedalTimer = 0;

void ResetScrambleTimer()
{
      scrambleTimerMS = millis()+scrambleStepMS;
}

void UpdateScrambler()
{
   if (scrambleTimerMS < millis())
   {
     //time to do it
     //TinyBeep();
     if (bLastScrambleOn)
     {
      if (HDMI_PIN != 0)
      {
      //turn stuff off
        digitalWrite(HDMI_PIN, HDMI_OFF);  //turn it on
      }
       if (HDMI_SECOND_PIN != 0)
       digitalWrite(HDMI_SECOND_PIN, HDMI_OFF);  //turn it on
      
     } else
     {
      //turn stuff back on
      if (HDMI_PIN != 0)
      {
       digitalWrite(HDMI_PIN, HDMI_ON);  //turn it on
      }
       if (HDMI_SECOND_PIN != 0)
       digitalWrite(HDMI_SECOND_PIN, HDMI_ON);  //turn it on
     }

     bLastScrambleOn = !bLastScrambleOn;

    //set timer for next time
    ResetScrambleTimer();
   }
  
}

int SmoothedPedalRead(int pedalPin)
{
  if (pedalPin == 0) return 0;
  
   int val = digitalRead(pedalPin);  // read input value
 
   if (lastSmoothedPedalRead == val) 
   {
    smoothedPedalTimer = millis();
    return val; //nothing has changed
   }

   //something has changed.  But we'll only respect it if we've waited long enough
   if (smoothedPedalTimer+smoothedPedalRequiredTimeMS < millis())
   {
    //we've waited long enough, we have a new value, folks!
    lastSmoothedPedalRead = val;
    smoothedPedalTimer = millis(); //make sure it doesn't switch again too quick
   }

   return lastSmoothedPedalRead;
}

// the loop function runs over and over again forever
void loop() 
{
  if (stateTimerMS != 0)
  {
    if (stateTimerMS < millis())
    {
      stateTimerMS = millis()+stateTimeBetweenStatesMS;
      RunDelayedStartup();
    }
  }

  if (stateStartup != STARTUP_FINISHED)
  {
    return;
  }
  
  int val = SmoothedPedalRead(PEDAL_PIN);
   
    //Serial.print("Read ");
    //Serial.println(val, DEC);
  
  if (val != bLastPedalHigh && (timerLastStepMS+DEBOUNCE_TIMER_MS) < millis())
  {
    bLastPedalHigh = val;
    if (energy == 0)
    {
      bLastScrambleOn = true;
       digitalWrite(HDMI_PIN, HDMI_ON);  //turn it on
       if (HDMI_SECOND_PIN != 0)
       digitalWrite(HDMI_SECOND_PIN, HDMI_ON);  //turn it on
   }

    energy += ENERGY_EACH_STEP_GIVEN;
    if (energy < 2)
    {
      //for the very first one, give them an extra second, otherwise they can nearly instantly hear the game over sound
      energy = 4;
    }
    
    timerLastStepMS = millis();
    
    if (energy > MAX_ENERGY)
    {
      energy = MAX_ENERGY;
      PlayEnergyFullSound();
    }
    
    PlayGetEnergySound();
    DrawEnergy();
  }

    if (LED_PIN != 0)
  {
    if (val == HIGH)
    {         // check if the input is HIGH (button released)
      digitalWrite(LED_PIN, LOW);  // turn LED OFF
    } else 
    {
      digitalWrite(LED_PIN, HIGH);  // turn LED ON
    }
  }

  if ( millis() > timerSecondTickMS)
  {
    timerSecondTickMS = millis()+ONE_SECOND_MS;
    EverySecondUpdate();
  }

if (energy <= 0)
  UpdateScrambler();

  delay(10);
  
}


void EverySecondUpdate()
{
  if (energy <= 0)
  {
    //already at zero energy, nothing we can do
    return;
  }

  energy -= 1;
  
  DrawEnergy();

  if (energy <= 0)
  {
    PlayOutOfEnergySound();
    digitalWrite(HDMI_PIN, HDMI_OFF);  //turn it off
    if (HDMI_SECOND_PIN != 0)
      digitalWrite(HDMI_SECOND_PIN, HDMI_OFF);  //turn it off
  return;
  }

  //should we play warning sounds?
  if (energy < WARNING_TIME_SECONDS)
  {
    //don't play the warning it if they've JUST added a step, it's disconcertuing
    if (timerLastStepMS+500 < millis())
    {
        PlayWarning(energy);
    }
  }
}


void PlayWarning(int energyLeft)
{

 int levelAscending = WARNING_TIME_SECONDS-energyLeft;
 
#ifdef DEBUG
Serial.print("warning level: ");
Serial.println(levelAscending, DEC);
#endif
  beep(200+ (energyLeft)*20, 100);
}

//for debugging
void TinyBeep()
{
   beep(NOTE_G4, 30);
 
}
void PlayOutOfEnergySound()
{
  beep(NOTE_G4, 100);
  beep(NOTE_F4, 100);
  beep(NOTE_D4, 100);
  beep(NOTE_C4, 200);
 
}

//Sounds below here taken or modified from the Volume Library Demo Sketch (c) 2016 Connor Nishijima which is released under
//released under the GPLv3 license.   https://github.com/connornishijima/arduino-volume


void PlayGetEnergySound()
{

  if (SPEAKER_PIN == 0) return;
  
  tone(SPEAKER_PIN, 1025, 255);
  delay(20);
  int v = 100;
  while(v > 0)
  {
    tone(SPEAKER_PIN, 2090,v); // ting!
    delay(3);
    v -=(3);
  }
}

void PlayEnergyFullSound()
{
  if (SPEAKER_PIN == 0) return;
  
  int f = 122; // starting frequency
  int v = 0;   // starting volume
  while (f < 4000) 
  {  // slide up to 4000Hz
    tone(SPEAKER_PIN,f, v);
    v = 255 * (f / 4000.00);
    f += 25;
    delay(1);
  }
  noTone(SPEAKER_PIN);
  delay(100); // wait a moment
  f = 122; // starting frequency
  v = 0;   // starting volume
  while (f < 3000)
  { // slide up to 3000Hz
    tone(SPEAKER_PIN,f, v);
    v = 255 * (f / 4000.00); 
    f += 25;
    delay(2);
  }
  while (f > 125) 
  { // slide down to 125Hz
    tone(SPEAKER_PIN,f, v);
    v = 255 * (f / 4000.00);
    f -= 25;
    delay(2);
  }
  
  noTone(SPEAKER_PIN);
}

void beep(int note, int duration)
{
  if (SPEAKER_PIN == 0) return;
  
  if (note != 0)
  {
    tone(SPEAKER_PIN, note, duration);
  }
  
  delay(duration);
  noTone(SPEAKER_PIN);
}
