# Arduino-timed_lights

For home automation
Low power sketch for **ESP12-e** with **DS3231** RTC module

**Functionalities**
  - Have two sets of alarm, AM and PM
    - ***AM*** for switching off the relay that powers the target appliance and for making the ESP12-e sleep
    - ***PM*** for waking up the ESP12-e and turning ON the relay that powers the appliance
  - Each alarm can be configured using the syntax _"SA <hh:mm>"_ on the serial monitor
    - _SA_: For set alarm, use 24-hour format
    - _Example_: "SA 11:59" (This will set the alarm at 11:59 AM, alarm will always recur) 
    - _Example_: "SA 14:00" (This will set the alarm at 2:00 PM, alarm will always recur)
  - The time can be adjusted using the syntax "SCT <yyyy/MM/dd hh/mm/ss>
    - _SCT_: For set current time, use 24-hour format
    - _Example_: "SCT 2022/11/01 14:01:59" (This will set the date and time to the parameter)
    
**Setup**
The following are the ESP12-e pin assignments:
 - D1 -> SCL pin of DS3231
 - D2 -> SDA pin of DS3231
 - D3 -> SQW pin of DS3231
 - D5 -> Boost converter pin
 - D6 -> Relay pin
