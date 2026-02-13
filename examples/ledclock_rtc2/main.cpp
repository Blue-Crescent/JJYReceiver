#include <Arduino.h>
#include <TM1637.h> // smougenot/TM1637 (TM1637Display)
#include <Ds1302.h>        // treboada/Ds1302
#include <JJYReceiver.h>
#include <MsTimer2.h>


// TM1637 ピン設定
#define CLK2 9
#define DIO2 10

// DS1302 ピン設定 (RST, CLK, DAT)
#define RTC_RST A1
#define RTC_CLK A5
#define RTC_DAT A4

// JJY ピン設定
#define DATA 2
#define SEL 7
#define PON 3

//TM1637MIRROREDDisplay led2(CLK, DIO);
TM1637 led(CLK2, DIO2);
Ds1302 rtc(RTC_RST, RTC_CLK, RTC_DAT);
JJYReceiver jjy(DATA, SEL, PON);

// treboada/Ds1302ライブラリの時刻保持用構造体
Ds1302::DateTime now;

// DateTime now;
static auto d = DisplayDigit().setG();
static uint8_t rawBuffer[4] = {0, 0, 0, 0};
uint8_t counter = 0;
uint16_t wait_counter = 0;
uint8_t firstreception = 1;
uint8_t rtcupdate = 1;


void isr_routine() {
  jjy.jjy_receive();
}

void ticktock() {
  jjy.delta_tick();
  wait_counter++;
}

void delay_nonblk(uint16_t msec10) {
  wait_counter = 0;
  while (wait_counter < msec10) {
    jjy.getTime(); // 待ち時間中もデコード処理を継続
  }
}


void printYear(){
    char buf[5];
    sprintf(buf, "%d", 2000 + (int)now.year);
    led.display(String(buf));
}
void printDate(){
    char buf[5];
    if(now.month < 10){
      sprintf(buf, "%d %2d", now.month, now.day);
    }else{
      sprintf(buf, "%d%2d", now.month, now.day);
    }
    led.display(String(buf));
}
void printTime(){
    char buf[5];
    if(now.hour < 10){
      sprintf(buf, " %d%02d", now.hour, now.minute);
    }else{
      sprintf(buf, "%d%02d", now.hour, now.minute);
    }
    led.display(String(buf));
}
void printDay(){
    led.display("    ");
    rawBuffer[0] = 0;
    rawBuffer[1] = 0;
    rawBuffer[2] = 0;
    rawBuffer[3] = 0;
    switch(now.dow){
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

void setRTC() {
  time_t jjytime = jjy.get_time();
  struct tm t;
  gmtime_r(&jjytime, &t);

  Ds1302::DateTime dt;
  dt.year = t.tm_year - 100; // 125(2025年) -> 25
  dt.month = t.tm_mon + 1;
  dt.day = t.tm_mday;
  dt.hour = t.tm_hour;
  dt.minute = t.tm_min;
  dt.second = t.tm_sec;
  dt.dow = t.tm_wday;

  rtc.setDateTime(&dt);
}

void setup() {
  // LGT8f328等の特殊ピン処理がある場合はここを維持
  pinMode(8, INPUT); pinMode(DIO2, OUTPUT);
  pinMode(CLK2, OUTPUT); pinMode(11, INPUT);
  pinMode(4, INPUT); pinMode(CLK, OUTPUT);
  pinMode(DIO, OUTPUT); 

  pinMode(DATA, INPUT);
  pinMode(SEL, OUTPUT);
  pinMode(PON, OUTPUT);

  MsTimer2::set(10, ticktock);
  MsTimer2::start();
  attachInterrupt(digitalPinToInterrupt(DATA), isr_routine, CHANGE);

  led.setBrightnessPercent(100);

  // led2.setBrightness(0x01);
  rtc.init();


  // RTCが動いていない、またはデータが不正な場合の初期設定
  if (rtc.isHalted()) { // 簡易的な妥当性チェック
    Ds1302::DateTime dt = { 92, 6, 20, 0, 12, 0, 3 }; // 2025/03/05 Wed
    rtc.setDateTime(&dt);
  }

  jjy.begin();
  //jjy.freq(60);
}

void loop() {
  rtc.getDateTime(&now);
  time_t lastreceived = jjy.getTime();

  if(lastreceived != -1){
    led.switchColon();

    if(now.second == 0){
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
  if(now.minute == 0 && lastreceived != -1){ // receive from last over an hour.
    jjy.begin();
    rtcupdate = 1;
  } 
}