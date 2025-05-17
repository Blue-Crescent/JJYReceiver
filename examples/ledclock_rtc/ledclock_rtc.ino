#include <Arduino.h>
#include <TM1637.h>
#include <RTClib.h>
#include <Wire.h>
#define SWAPFREQ
#include <JJYReceiver.h>
#include <MsTimer2.h>

//	akj7/TM1637 Driver@^2.2.1
//	blue-crescent/JJYReceiver@^0.11.2
//	paulstoffregen/MsTimer1@^1.1
//	adafruit/RTClib@^2.1.4

#define CLK 10
#define DIO 9

#define DATA 2
#define SEL 13
#define PON 12

TM1637 led(CLK, DIO);
JJYReceiver jjy(DATA,SEL,PON);
RTC_DS1307 rtc;

DateTime now;
static auto d = DisplayDigit().setG();
static uint8_t rawBuffer[4] = {0, 0, 0, 0};
uint8_t counter=0;
uint16_t wait_counter=0;
uint8_t firstreception = 1;
uint8_t rtcupdate= 1;

void isr_routine() { // pin change interrupt service routine
  jjy.jjy_receive(); 
}
void ticktock() {  // 10 msec interrupt service routine
  jjy.delta_tick();
  wait_counter = wait_counter + 1;
}

// wait function. (argument granurarity is 10msec)
void delay_nonblk(uint16_t msec10){
    wait_counter = 0;
    while(wait_counter < msec10){
      jjy.getTime(); // Calling for realtime mktime calculation on main thread while waiting.
    }
}

void printYear(){
    char buf[5];
    sprintf(buf, "%d", now.year());
    led.display(String(buf));
}
void printDate(){
    char buf[5];
    if(now.month()<10){
      sprintf(buf, "%d %2d", now.month(),now.day());
    }else{
      sprintf(buf, "%d%2d", now.month(),now.day());
    }
    led.display(String(buf));
}
void printTime(){
    char buf[5];
    if(now.hour()<10){
      sprintf(buf, " %d%02d", now.hour(),now.minute());
    }else{
      sprintf(buf, "%d%02d", now.hour(),now.minute());
    }
    led.display(String(buf));
}
void printDay(){
    led.display("    ");
    rawBuffer[0] = 0;
    rawBuffer[1] = 0;
    rawBuffer[2] = 0;
    rawBuffer[3] = 0;
    switch(now.dayOfTheWeek()){
      case(0): d = DisplayDigit().setE(); rawBuffer[0] = d;break;
      case(1): d = DisplayDigit().setC(); rawBuffer[0] = d;break;
      case(2): d = DisplayDigit().setE(); rawBuffer[1] = d;break;
      case(3): d = DisplayDigit().setC(); rawBuffer[1] = d;break;
      case(4): d = DisplayDigit().setE(); rawBuffer[2] = d;break;
      case(5): d = DisplayDigit().setC(); rawBuffer[2] = d;break;
      case(6): d = DisplayDigit().setE(); rawBuffer[3] = d;break;
    }
    led.displayRawBytes(rawBuffer, 4);
}
void printFreq(){
  led.colonOn();
  char buf[5];
  sprintf(buf, " F%d", jjy.frequency);
  led.display(String(buf));
}
void printCalendar(){
  led.colonOff();
  printYear();
  delay_nonblk(209);
  printDate();
  delay_nonblk(200);
  printDay();
  delay_nonblk(209);
  led.colonOn();
  printTime();
  delay_nonblk(200);
}
void setRTC(){
  tm tm_info2;
  time_t jjytime = jjy.get_time();
  localtime_r(&jjytime, &tm_info2);
  int year   = tm_info2.tm_year + 1900;
  int month  = tm_info2.tm_mon + 1;
  int day    = tm_info2.tm_mday;
  int hour   = tm_info2.tm_hour;
  int minute = tm_info2.tm_min;
  int second = tm_info2.tm_sec;
  DateTime dt(year, month, day, hour, minute, second);
  rtc.adjust(dt);
  now = rtc.now();
}
void setup() {
  // For LGT8f328 board pin 8 and 9, pin 10 & 11 are shared pin.
  pinMode(8, INPUT);  pinMode(DIO, OUTPUT);  
  pinMode(CLK, OUTPUT); pinMode(11, INPUT);

  // JJY receiver module port setup.
  pinMode(DATA, INPUT);
  pinMode(SEL,OUTPUT);
  pinMode(PON, OUTPUT);
  
 // 10msec Timer for clock ticktock (Mandatory)
  MsTimer2::set(10, ticktock);
  MsTimer2::start();
  
  // DATA pin signal change edge detection. (Mandatory)
  attachInterrupt(digitalPinToInterrupt(DATA), isr_routine, CHANGE);
  Wire.begin();
  rtc.begin();

  // Clock output quiescence for noize reduction 
  Wire.beginTransmission(0x68);
  Wire.write(0x07);           // RTC clock output stop
  Wire.write(0x00);           // SQWE = 0, OUT = 0
  Wire.endTransmission();

  if (! rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    while (true);
  }
  // rtc.adjust(DateTime(2014, 1, 2, 3, 0, 0));
  // JJY Library
  jjy.begin(); // Start JJY Receive
}
void loop() {
  now = rtc.now();
  time_t lastreceived = jjy.getTime();

  if(lastreceived != -1){
    led.switchColon();

    if(now.second()==0){
      // Display date every 00 second
      printCalendar();
    }else{
      // Display current time
      printTime();
      delay_nonblk(50);
    }
    if (rtcupdate == 1){
      // Reception 
      setRTC();
      led.colonOff();
      led.display("Good");
      delay_nonblk(200);
      printCalendar();
      rtcupdate = 0;
    }
    firstreception = 0;
  }else{
    if (firstreception == 1){ // Display receiving quality
      counter = (counter + 1) % 25;
      if(counter == 24) {
        printCalendar();
        printFreq();
        delay_nonblk(200);
      }else{
        // Display reception quality level.
        led.colonOff();
        switch(counter%4){
          case(0): d = DisplayDigit().setG(); break;
          case(1): d = DisplayDigit().setC(); break;
          case(2): d = DisplayDigit().setD(); break;
          case(3): d = DisplayDigit().setE(); break;
        }
        rawBuffer[0] = d;
        char buf[5];
        sprintf(buf, "%4d", jjy.quality); 
        led.display(String(buf));
        led.displayRawBytes(rawBuffer, 1);
        delay_nonblk(25);
      }
    }else{
      led.colonOn();
      printTime();
      delay_nonblk(50);
    }
  }
  if(now.minute() == 0 && lastreceived != -1){ // receive from last over an hour.
    jjy.begin();
    rtcupdate = 1;
  } 
}