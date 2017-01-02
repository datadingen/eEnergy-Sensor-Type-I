/****************************************************************************
  *This module contains specific functions for the Microchip RN2483 LoRa
  *module. The module communicates with the MCU via SPI. The SPI uses for
  *communication the pins:
  *
  *- 13 = SCK
  *- 12 = MISO
  *- 11 = MOSI
  *
  *During sending a LED will light.
  *
  *Author: Richard Cuperus
  *Date:   05-11-2016
*****************************************************************************/

/*Header files*/
#include "settings.h"     //Header file with LoRa settings
#include "debug.h"        //Header file with LoRa settings
#include "P1.h"           //Header file with LoRa settings
#include "lora.h"         //Header file with LoRa settings
#include "keys.h"         //Header file with LoRa keys
#include "wdt.h"          //Header file with LoRa settings

#define DEBUG             //Macro definition for the debugmode

/*Macrosubstitutes for CPU with AVR Architecture*/
#ifdef ARDUINO_ARCH_AVR                                                       
  #define LED LED1        //Pin ID for LED

/*Macrosubstitutes for CPU with SAM D Architecture*/
#elif ARDUINO_ARCH_SAMD                                                       
  #define LED LED_BUILTIN //Pin ID for LED
#endif

/*Macrosubstitutes for pins*/
#define P1_RTS         5  //Pin for Request To Send (RTS)
#define BAT_SENSOR_PIN A5 // 

/*Global variables*/
bool 
  softReset = false,     //
  firstRun  = true;      //

char
  payload[29];           //29 characters with "0"

int
  batteryValue,          //
  sleepRemainingS = 0;   //

/****************************************************************************
  *
*****************************************************************************/ 
void setup()
{
  disableWDT; //Disable watchdog timer to prevent start up loop after soft reset
         
  pinMode(LED, OUTPUT);        //Setup LED, on by default
  pinMode(P1_RTS, OUTPUT);     //Setup RTS for sensor
  digitalWrite(BEE_VCC, HIGH); //Supply power to lora bee
  
#ifdef DEBUG
  setupDebug;
  //LoRaBee.setDiag(debugSerial);
#endif

  setupLoRa;
  initPayload;
    
  setupP1; /*Setup serial port for communication with smart meter*/
    
  enableWDT;//Enable WDT and set the wakeup interval to 16s
  
}

/****************************************************************************
  ...
*****************************************************************************/
void loop()
{
  timeoutHandled;//mark wdt timeout as handled
  sleepRemainingS -=16;   //substract wdt interval

  if(softReset) 
  {
    static bool resetTriggered = false;
    
    /*Prevent code from executing multiple times*/
    if(!resetTriggered) 
    {                                                     
      resetTriggered = true;
      logMsg("Soft reset triggered!");
    }
    return;
  }
  else
  {
    resetWDT;
    
    digitalWrite(LED, HIGH); //Light the LED
    
    if(sleepRemainingS <= 0)
    {
      /*Set current batterylevel*/
      batteryValue = int ((analogRead(BAT_SENSOR_PIN)/255)*253);
      setBatteryLevel(batteryValue);

      /*Request data from smart meter*/
      logMsg("Requesting data...");
      digitalWrite(P1_RTS, HIGH);
      getDataP1;
      digitalWrite(P1_RTS, LOW);

      sendMsg(); //Send data
      
      sleepRemainingS = WAKEUP_INTERVAL_S;
    }
    else
    {
      delay(100);
    }
    
    digitalWrite(LED, LOW); //End lightning the LED
  }
  resetWDT;

#ifdef DEBUG
  delay(8000); //Instead of sleeping, delay with watchdog interval
#else
  systemSleep();
#endif
}


