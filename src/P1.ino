/****************************************************************************
  This module contains functions for the watchdog timer.
  
  Author: Richard Cuperus
  Date:   26-12-2016
  
*****************************************************************************/

/****************************************************************************
  ...
*****************************************************************************/
void getDataP1() 
{
  /*Get data from P1 serial*/
  String line = "";
  do {
    line = P1Serial.readStringUntil('\n');           //Warning: timeout does not seem to work
    if(line == NULL || line.length() == 0) continue;
    logMsg("Received: " + line);
    parseInput(line);
  } while(line == NULL || !line.startsWith("!"));    //Read until termination line is encountered
}

/****************************************************************************
  ...
*****************************************************************************/
void setupP1()
{
    /*while(!P1Serial);
    P1Serial.begin(9600, SERIAL_7N1);
    P1Serial.setTimeout(1000);*/
    
  /*Setup serial port with correct baudrate, parity ansd start- and stopbits*/
  if(firstRun) { //Setup serial port only once at startup
    char b[1];
    P1Serial.readBytes(b,1);
    if(b[0] != '/') 
    {
      P1Serial.begin(115200, SERIAL_8N1);
      P1Serial.setTimeout(1000);
    }
  }
  firstRun = false;
}

/****************************************************************************
  ...
*****************************************************************************/
void parseInput(String line)
{
  logMsg("Parsing: " + line);

  /*extract OBIS reference and value*/
  int firstParenthesisIndex = line.indexOf('(', 4);
  int lastParenthesisIndex = line.lastIndexOf('(');
  int asteriskIndex = line.indexOf('*', lastParenthesisIndex);

  if(firstParenthesisIndex < 0 || asteriskIndex < 0) return;
  
  String OBISref = line.substring(4, firstParenthesisIndex);
  String value = line.substring(lastParenthesisIndex + 1, asteriskIndex);
  
  /*Convert value to long*/
  int decimalPointIndex = value.indexOf('.');
  String pointlessValue = value.substring(0, decimalPointIndex) + value.substring(decimalPointIndex + 1);
  char pointlessArray[pointlessValue.length()+1];
  pointlessValue.toCharArray(pointlessArray, pointlessValue.length()+1);
  unsigned long amount = atol(pointlessArray); //WARN: conversion to long before ulong?

  logMsg(OBISref + " = " + String(amount));

  /*Determine position of value in payload*/
  int pos;
  if(OBISref.equals("1.8.1")) pos = 1;
  else if(OBISref.equals("1.8.2")) pos = 5;
  else if(OBISref.equals("2.8.1")) pos = 9;
  else if(OBISref.equals("2.8.2")) pos = 13;
  else if(OBISref.equals("1.7.0")) pos = 17;
  else if(OBISref.equals("2.7.0")) pos = 21;
  else if(OBISref.equals("24.2.1")) pos = 25;
  else return;
    
  /*Insert value in payload at pos*/
  for(int i = 0; i <= 3; i++)
  {
    char c = 0xFF & amount >> 8*i;
    payload[pos + 3 - i] = c;
    logMsg("char " + String(c) + " at " + String(pos + 3 - i));
  }

  logMsg("Updated payload: ");
  for(int i = 0; i<sizeof(payload); i++)
  {
    logMsgNoLn(String(payload[i]));
  }
  logMsg("");
}

