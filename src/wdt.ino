/****************************************************************************
  This module contains functions for the watchdog timer.
  
  Author: Richard Cuperus
  Date:   26-12-2016
  
*****************************************************************************/

/****************************************************************************
  ...
*****************************************************************************/
void timeoutHandled()
{
  sodaq_wdt_flag = false; //mark wdt timeout as handled
}
/****************************************************************************
  ...
*****************************************************************************/
void resetWDT()
{
  sodaq_wdt_reset();
}

/****************************************************************************
  ...
*****************************************************************************/
void disableWDT()
{
  sodaq_wdt_disable();  
}

/****************************************************************************
  ...
*****************************************************************************/
void enableWDT()
{
  sodaq_wdt_enable(WDT_PERIOD_8X); //Enable WDT and set the wakeup interval to 16s
}

/****************************************************************************
//Sleep until WDT interrupt
*****************************************************************************/
void systemSleep()
{
#ifdef DEBUG
  debugSerial.flush();
#endif
  
/*Implement a watchdog timer on CPU's with AVR architecture*/ 
#ifdef ARDUINO_ARCH_AVR                                                       
  ADCSRA &= ~_BV(ADEN); //ADC disabled
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);                                        

  cli();  
  
  if (!sodaq_wdt_flag) //Only go to sleep if there was no watchdog interrupt.
  {
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
  }
  sei();

  ADCSRA |= _BV(ADEN); //ADC enabled

/*Implement a watchdog timer on CPU's with SAM D architecture*/
#elif ARDUINO_ARCH_SAMD                                                       
  if (!sodaq_wdt_flag)                 //Only go to sleep if there was no watchdog interrupt.
  {
    USBDevice.detach();                //Disable USB
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //Set the sleep mode
    __WFI();                           //Go to sleep
  }

#endif
}

