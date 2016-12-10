#include <Sodaq_RN2483.h>
#include <Sodaq_wdt.h>

#define debugSerial SerialUSB
#define loraSerial Serial1
#define P1Serial Serial
#define LED LED_BUILTIN
#define P1_RTS 5

const byte VERSION = 0;

/*
const String data[] = {
  "/KFM5KAIFA-METER",
  "",
  "1-3:0.2.8(42)",
  "0-0:1.0.0(160927163554S)",
  "0-0:96.1.1(4530303236303030303234343334363135)",
  "1-0:1.8.1(000382.096*kWh)",
  "1-0:1.8.2(000378.975*kWh)",
  "1-0:2.8.1(000000.000*kWh)",
  "1-0:2.8.2(000000.000*kWh)",
  "0-0:96.14.0(0002)",
  "1-0:1.7.0(00.329*kW)",
  "1-0:2.7.0(00.000*kW)",
  "0-0:96.7.21(00001)",
  "0-0:96.7.9(00001)",
  "1-0:99.97.0(1)(0-0:96.7.19)(000101000002W)(2147483647*s)",
  "1-0:32.32.0(00000)",
  "1-0:52.32.0(00000)",
  "1-0:72.32.0(00000)",
  "1-0:32.36.0(00000)",
  "1-0:52.36.0(00000)",
  "1-0:72.36.0(00000)",
  "0-0:96.13.1()",
  "0-0:96.13.0()",
  "1-0:31.7.0(000*A)",
  "1-0:51.7.0(001*A)",
  "1-0:71.7.0(000*A)",
  "1-0:21.7.0(00.105*kW)",
  "1-0:41.7.0(00.200*kW)",
  "1-0:61.7.0(00.024*kW)",
  "1-0:22.7.0(00.000*kW)",
  "1-0:42.7.0(00.000*kW)",
  "1-0:62.7.0(00.000*kW)",
  "0-1:24.1.0(003)",
  "0-1:96.1.0(4730303139333430323835313836313136)",
  "0-1:24.2.1(160927160000S)(00097.538*m3)",
  "!D024"
};
*/

char payload[29]; //29*0
bool firstRun = true;

void onSetup()
{
  pinMode(P1_RTS, OUTPUT);
  
  while(!P1Serial);
  P1Serial.begin(9600, SERIAL_7N1);
  P1Serial.setTimeout(1000);

  //fill payload with 0xFF
  for(int i = 0; i<sizeof(payload); i++) {
    payload[i] = 0xFF;
  }
}

void onWakeup()
{
  /*
  //read fake data
  for(int i = 0; i<9; i++)
  {
    parseInput(data[i]);
  }
  */

  logMsg("Requesting data...");
  digitalWrite(P1_RTS, HIGH);
  
  //Set correct serial baud and parity, both 9600, 7n1 and 115200, 8n1 are possible depending on the smartmeter
  //logMsg("Requesting data...");
  if(firstRun) {
    char b[1];
    P1Serial.readBytes(b,1);
    if(b[0] != '/') P1Serial.begin(115200, SERIAL_8N1);
  }
  firstRun = false;

  //get data from P1 serial
  String line = "";
  do {
    line = P1Serial.readStringUntil('\n'); //WARN: timeout does not seem to work
    if(line == NULL || line.length() == 0) continue;
    logMsg("Received: " + line);
    parseInput(line);
  } while(line == NULL || !line.startsWith("!")); //read until termination line is encountered
  digitalWrite(P1_RTS, LOW);
  
/*  for(int i = 0; i <= 3; i++)
  {
    char c = 0xFF;
    payload[3 - i] = c;
    logMsg("char " + String(c) + " at " + String(3 - i));
  }*/
  
  sendMsg();
}

void parseInput(String line)
{
  logMsg("Parsing: " + line);

  //extract OBIS reference and value
  int firstParenthesisIndex = line.indexOf('(', 4);
  int lastParenthesisIndex = line.lastIndexOf('(');
  int asteriskIndex = line.indexOf('*', lastParenthesisIndex);

  if(firstParenthesisIndex < 0 || asteriskIndex < 0) return;
  
  String OBISref = line.substring(4, firstParenthesisIndex);
  String value = line.substring(lastParenthesisIndex + 1, asteriskIndex);
  
  //convert value to long
  int decimalPointIndex = value.indexOf('.');
  String pointlessValue = value.substring(0, decimalPointIndex) + value.substring(decimalPointIndex + 1);
  char pointlessArray[pointlessValue.length()+1];
  pointlessValue.toCharArray(pointlessArray, pointlessValue.length()+1);
  unsigned long amount = atol(pointlessArray); //WARN: conversion to long before ulong?

  logMsg(OBISref + " = " + String(amount));

  //determine position of value in payload
  int pos;
  if(OBISref.equals("1.8.1")) pos = 1;
  else if(OBISref.equals("1.8.2")) pos = 5;
  else if(OBISref.equals("2.8.1")) pos = 9;
  else if(OBISref.equals("2.8.2")) pos = 13;
  else if(OBISref.equals("1.7.0")) pos = 17;
  else if(OBISref.equals("2.7.0")) pos = 21;
  else if(OBISref.equals("24.2.1")) pos = 25;
  else return;
    
  //insert value in payload at pos
  for(int i = 0; i <= 3; i++)
  {
    char c = 0xFF & amount >> 8*i;
    payload[pos + 3 - i] = c;
    logMsg("char " + String(c) + " at " + String(pos + 3 - i));
  }

  debugSerial.print("Updated payload: ");
  for(int i = 0; i<sizeof(payload); i++)
  {
    logMsgNoLn(String(payload[i]));
  }
  logMsg("");
}
  
void sendMsg() {
  logMsg("Sending message...");
  payload[0] = VERSION;
  LoRaBee.send(1, (uint8_t*)payload, sizeof(payload));
}
