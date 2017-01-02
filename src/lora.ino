/****************************************************************************
  This is a build of the eDevice for measuring the ambient temperature in a
  room. This is also called the referencetemparature.

  This software is designed specifically to work with the ... Sodaq Autonomo
  and LoRabee LoRa shield.

  //http://downloads.sodaq.net/package_sodaq_index.json

  The LoRabee shield communicates with the Arduino via SPI. The SPI uses for
  communication the pins:

  - 13 = SCK
  - 12 *= MISO
  - 11 = MOSI

  Author: Richard Cuperus
  Date:   05-11-2016
  
/****************************************************************************
  ...
*****************************************************************************/
void initPayload() 
{
  /*Fill payload with 0xFF*/
  for(int i = 0; i<sizeof(payload); i++) {
    payload[i] = 0xFF;
  }
}

/****************************************************************************
  Setup LoRa connection
*****************************************************************************/
void setupLoRa() 
{
  /**/
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
    setDataRate(0); //Hack because no acknoledgements received                                                          
  }
}

/****************************************************************************
  ...
*****************************************************************************/
void setDataRate(int dr) 
{
  loraSerial.print("mac set dr 0\r\n");
  
  String result = loraSerial.readString();
  logMsg("Setting data rate to " + String(dr) + ": " + result);
}

/****************************************************************************
  ...
*****************************************************************************/
void setBatteryLevel(int bat)
{
  loraSerial.print("mac set bat " + String(bat) + "\r\n");
  
  String result = loraSerial.readString();
  logMsg("Setting battery level to " + String(bat) + ": " + result);
}

/****************************************************************************
  ...
*****************************************************************************/
void sendMsg() 
{
  logMsg("Sending message...");
  LoRaBee.send(1, (uint8_t*)payload, sizeof(payload));
}

