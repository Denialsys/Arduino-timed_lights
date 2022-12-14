/*
 * Author: Jenver I.
 * 
 * For DS3231 documentations 
 * Visit: https://adafruit.github.io/RTClib/html/_r_t_clib_8h.html#abf9692bbba5b3b3adbb6939fe05c0cb3
 * 
 * Sketch for using the DS3231 with the ESP12e
 * Used home appliance automation
 * 
 * This will have the functions to:
 *    - set the current time
 *    - set the alarms for AM and PM alarms
 *    - clear alarms
 *    - get the alarm status
 *    - activate the boost and relay then deactivate boost again
 *    - deactivate boost and relay
 *  
 * To do:
 *    - add the function for sleeping and waking up
 *    - break the sketch into multiple tabs
 */


#include <RTClib.h>
#include "nodemcu_pins.h"
RTC_DS3231 rtc;// declaring the RTC module

String inputString = "";         // a String to hold incoming data
volatile bool stringComplete = false;  // whether the string is complete

#define alarmPin D5
#define boostPin D6
#define relayPin D7

#define CLOCK_INTERRUPT_PIN 2

unsigned long lastTimeDisplay = 0;

void ICACHE_RAM_ATTR lowPowerSleep(); //Add to cache the ISR (Only for ESP12e)

void setup() {

  Serial.begin(9600);
  
  if(!rtc.begin()) {
      Serial.println("Couldn't find RTC!");
      Serial.flush();
      while (1) delay(10);
  }
  
  if(rtc.lostPower()) {
      Serial.println("Power was lost in RTC please set the date and time");
  }
  
  // we don't need the 32K Pin, so disable it
  rtc.disable32K();

  // We'll use the SQW pin for alarm so disable the pwm
  rtc.writeSqwPinMode(DS3231_OFF);
  
  pinMode(alarmPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(alarmPin), lowPowerSleep, FALLING);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) serialEvent();

  if (inputString.indexOf("SCT") > -1){
    setCurrentTime();
    
  }else if (inputString.indexOf("SA") > -1){
    setAlarm();

  }else if (inputString.indexOf("GA") > -1){
    getAlarms();

  }else {

    if(stringComplete) {
      Serial.println("Unrecognized command");
    }
  }
  
  // Clean up
  if (stringComplete) {
    inputString = "";
    stringComplete = false;
  }

  // Display the time update to the serial monitor
  displayTimeUpdate(2500);
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte add it to the inputString:
    char inChar = (char)Serial.read();
    inputString += inChar;
    
    //Serial data has ended parse
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void setCurrentTime(){
  
  if(!stringComplete) return;
  Serial.println("Setting the current time");
  char _dateTime[39]; 
  // Time format "yyyy/MM/dd hh/mm/ss"
  short spcIndex = inputString.indexOf(' ');
  String dateTime = inputString.substring(spcIndex+1);

  // Change the magic numbers into for loop
  short yyyy = dateTime.substring(0, 4).toInt();
  short MM = dateTime.substring(5, 7).toInt();
  short dd = dateTime.substring(8, 10).toInt();
  short hh = dateTime.substring(11, 13).toInt();
  short mm = dateTime.substring(14, 16).toInt();
  short ss = dateTime.substring(17, 19).toInt();
  
  rtc.adjust(
    DateTime(yyyy, MM, dd, hh, mm, ss)
  );

  sprintf(
    _dateTime,
    "Extracted datetime: %02d/%02d/%02d %02d:%02d:%02d",
    yyyy, MM, dd, hh, mm, ss
  );
  Serial.println(_dateTime);
}

void setAlarm(){

  if(!stringComplete) return;  
  Serial.println("Setting the alarm");

  if (inputString.indexOf("CA1") > -1){
    // Clear alarm 1
    rtc.clearAlarm(1);
    rtc.disableAlarm(1);
    Serial.println("Alarm 1 cleared");
  }
  if (inputString.indexOf("CA2") > -1){
    // Clear alarm 2
    rtc.clearAlarm(2);
    rtc.disableAlarm(2);
    Serial.println("Alarm 2 cleared");
  }
  if (inputString.indexOf("CA") > -1){
    // Terminate the function bec alarm will not be set
    return;
  }

  // Set the recurring alarm
  short spcIndex = inputString.indexOf(' ');
  short sepIndex = inputString.indexOf(':');
  short hr = inputString.substring(spcIndex, sepIndex).toInt();
  short mn = inputString.substring(sepIndex+1).toInt();
  DateTime rtcTime = rtc.now();

  // Not a an accurate decision but good enough
  // Set wether the alarm will be on alarm1 or alarm2
  Serial.print("Alarm ");
  if ((hr < 12) &&(mn < 60)){
    // AM
    rtc.setAlarm1 (DateTime(0,0,0, hr, mn, 0), DS3231_A1_Hour);
    Serial.print("1");
    
  }else{
    // PM
    rtc.setAlarm2 (DateTime(0,0,0, hr, mn, 0), DS3231_A2_Hour);    
    Serial.print("2");
    
  }
  Serial.print(" set to ");
  Serial.println(inputString.substring(spcIndex));
}

void displayTimeUpdate(short dispDelay){

  //Check if the delay has passed
  if (millis() - lastTimeDisplay > dispDelay){
    lastTimeDisplay = millis();
  }else{
    return;
  }
  
  DateTime rtcTime = rtc.now();
  char dateTime[41];
  
  sprintf(
    dateTime,
    "Current datetime: %04d/%02d/%02d %02d:%02d:%02d",
    rtcTime.year(),
    rtcTime.month(),
    rtcTime.day(),
    rtcTime.hour(),
    rtcTime.minute(),
    rtcTime.second()
  );
  Serial.println(dateTime);
}

void activateRelay(){
  // Adjust the boost voltage (UP)

  digitalWrite(boostPin, HIGH);
  delay(300);
  digitalWrite(relayPin, HIGH);

  // Relay has latched adjust the boost voltage (DOWN) to conserve energy
  digitalWrite(boostPin, LOW);
}

void deactivateRelay(){
  digitalWrite(boostPin, LOW);
  digitalWrite(relayPin, LOW);
}

void getAlarms(){

  // Limitation:
  // DS3231 Lib has no member function for checking if the alarm will trigger or not
  
  if(!stringComplete) return;  
  // Check if there are alarms registered
  DateTime alarm1 = rtc.getAlarm1();
  DateTime alarm2 = rtc.getAlarm2();
  char alarmStatus[36];
  
  //(<Alarm index> <hh:mm> - <has fired>/<mode>)
  sprintf(
    alarmStatus,
    "(1* %02d:%02d - %01d/%02d),\n(2* %02d:%02d - %01d/%02d)",
    alarm1.hour(), alarm1.minute(), rtc.alarmFired(1), rtc.getAlarm1Mode(),
    alarm2.hour(), alarm2.minute(), rtc.alarmFired(2), rtc.getAlarm2Mode()
  );
  
  Serial.println(alarmStatus);
}

void lowPowerSleep(){
  // Add checking if it is really time to sleep or to start up the switches
  deactivateRelay();
  Serial.println("Low power sleep");
  
}
