/*Macrosubstitutes for CPU's with AVR Architecture*/
#ifdef ARDUINO_ARCH_AVR                                                       
  #define debugSerial Serial    //Serial port for debugging

/*Macrosubstitutes for CPU's with SAM D Architecture*/
#elif ARDUINO_ARCH_SAMD                                                       
  #define debugSerial SerialUSB //Serial port for debugging
#endif

