#include <Sodaq_RN2483.h>
#include <Sodaq_wdt.h>
#include <math.h>

#define DEBUG

#ifdef ARDUINO_ARCH_AVR
  #define debugSerial Serial
  #define LED LED1
#elif ARDUINO_ARCH_SAMD
  #define debugSerial SerialUSB
  #define LED LED_BUILTIN
#endif
#define loraSerial Serial1

//global constants
//KPN Developement Environment
//const uint8_t DEV_ADDR[4] = { 0x14, 0x20, 0x3F, 0x2E };
//const uint8_t NWK_SKEY[16] = { 0xb4, 0x83, 0xc2, 0x13, 0x2d, 0x6f, 0xf1, 0x53, 0x74, 0x54, 0x47, 0x08, 0x3b, 0x5b, 0x3d, 0xe9 };
//const uint8_t APP_SKEY[16] = { 0x50, 0xb5, 0xdf, 0x46, 0x94, 0xf7, 0x2c, 0x2e, 0x55, 0x33, 0x0a, 0x55, 0xce, 0x69, 0x59, 0xd5 };

//KPN Production Environment
const uint8_t DEV_ADDR[4] = { 0x14, 0x00, 0x11, 0x85};
const uint8_t NWK_SKEY[16] = { 0x68, 0x20, 0x75, 0x77, 0xc5, 0x4d, 0xe5, 0xa4, 0xb7, 0xcb, 0x26, 0x85, 0x1b, 0xc0, 0x43, 0xfb};
const uint8_t APP_SKEY[16] = { 0x2c, 0x81, 0x03, 0x3b, 0xf8, 0x9d, 0xed, 0x8e, 0x60, 0xc2, 0x93, 0x22, 0x8c, 0x65, 0x2e, 0x82};

const int WAKEUP_INTERVAL_S = 30; //15 minutes

//globar vars
bool softReset = false;

void setup()
{
  //disable watchdog to prevent startup loop after soft reset
  sodaq_wdt_disable();

  // Setup LED, on by default
  pinMode(LED, OUTPUT);

  //supply power to lora bee
  digitalWrite(BEE_VCC, HIGH);
  
#ifdef DEBUG
  //Wait for debugSerial or 10 seconds
  while ((!debugSerial) && (millis() < 10000));
  debugSerial.begin(9600);
  LoRaBee.setDiag(debugSerial);
#endif

  //setup LORA connection
  while(!loraSerial);
  loraSerial.begin(LoRaBee.getDefaultBaudRate());
  
  if (!LoRaBee.initABP(loraSerial, DEV_ADDR, APP_SKEY, NWK_SKEY))
  {
    logMsg("Connection to the network failed!");
    softReset = true;
  }
  else
  {
    logMsg("Connection to the network successful.");
    setDataRate(0); //HACK because no acknoledgements received
    //sendTestPacket();
  }

  onSetup();

  // Enable WDT
  sodaq_wdt_enable(WDT_PERIOD_8X); //wakeup interval = 16s
}

int sleepRemainingS = 0;

void loop()
{
  sodaq_wdt_flag = false; //mark wdt timeout as handled
  sleepRemainingS-=16; //substract wdt interval

  if(softReset) {
    static bool resetTriggered = false;
    if(!resetTriggered) { //prevent code from executing multiple times
      resetTriggered = true;
      logMsg("Soft reset triggered!");
    }
    return;
  }
  else
  {
    sodaq_wdt_reset();

    //start LED flash
    digitalWrite(LED, HIGH);
    
    if(sleepRemainingS <= 0)
    {
      onWakeup();
      sleepRemainingS = WAKEUP_INTERVAL_S;
    }
    else
    {
      delay(100);
    }

    //end LED flash
    digitalWrite(LED, LOW);
  }

  sodaq_wdt_reset();
#ifdef DEBUG
  delay(8000); //instead of sleeping, delay with watchdog interval
#else
  systemSleep();
#endif
  
}

void sendTestPacket() {
  const uint8_t testPayload[] = { 0x30, 0x31, 0xFF, 0xDE, 0xAD };
  logMsg("Sending test packet...");
  LoRaBee.send(1, testPayload, 5);
}

void setDataRate(int dr) {
  loraSerial.print("mac set dr 0\r\n");
  String result = loraSerial.readString();
  logMsg("Setting data rate to " + String(dr) + ": " + result);
}

void logMsg(String msg)
{
  #ifdef DEBUG
  debugSerial.println(msg);
  #endif
}

void logMsgNoLn(String msg)
{
  #ifdef DEBUG
  debugSerial.print(msg);
  #endif
}

//Sleep until WDT interrupt
void systemSleep()
{
  #ifdef DEBUG
  debugSerial.flush();
  #endif
  
#ifdef ARDUINO_ARCH_AVR  

  ADCSRA &= ~_BV(ADEN);         // ADC disabled

  /*
    Possible sleep modes are (see sleep.h):
    #define SLEEP_MODE_IDLE         (0)
    #define SLEEP_MODE_ADC          _BV(SM0)
    #define SLEEP_MODE_PWR_DOWN     _BV(SM1)
    #define SLEEP_MODE_PWR_SAVE     (_BV(SM0) | _BV(SM1))
    #define SLEEP_MODE_STANDBY      (_BV(SM1) | _BV(SM2))
    #define SLEEP_MODE_EXT_STANDBY  (_BV(SM0) | _BV(SM1) | _BV(SM2))
  */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  /*
     This code is from the documentation in avr/sleep.h
  */
  cli();
  // Only go to sleep if there was no watchdog interrupt.
  if (!sodaq_wdt_flag)
  {
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
  }
  sei();

  ADCSRA |= _BV(ADEN);          // ADC enabled

#elif ARDUINO_ARCH_SAMD

  // Only go to sleep if there was no watchdog interrupt.
  if (!sodaq_wdt_flag)
  {
    // Disable USB
    USBDevice.detach();

    // Set the sleep mode
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    // SAMD sleep
    __WFI();
  }

#endif
}
