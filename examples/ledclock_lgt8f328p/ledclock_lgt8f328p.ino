#include <Arduino.h>
#include <TM1637.h>
#include <JJYReceiver.h>
#include <MsTimer2.h>

// TM1637.h
// https://github.com/AKJ7/TM1637

#define CLK 10
#define DIO 9

#define DATA 2
#define SEL 13
#define PON 12

TM1637 led(CLK, DIO);
JJYReceiver jjy(DATA,SEL,PON);

void isr_routine() { // pin change interrupt service routine
  jjy.jjy_receive(); 
}
void ticktock() {  // 10 msec interrupt service routine
  jjy.delta_tick();
}

void setup() {
  // For LGT8f328 board pin 8 and 9, pin 10 & 11 are shared pin.
  pinMode(8, INPUT);  pinMode(DIO, OUTPUT);  
  pinMode(CLK, OUTPUT); pinMode(11, INPUT);

  pinMode(DATA, INPUT);
  pinMode(SEL,OUTPUT);
  pinMode(PON, OUTPUT);
  
  led.setBrightnessPercent(50);
 // 10msec Timer for clock ticktock (Mandatory)
  MsTimer2::set(10, ticktock);
  MsTimer2::start();
  
  // DATA pin signal change edge detection. (Mandatory)
  attachInterrupt(digitalPinToInterrupt(DATA), isr_routine, CHANGE);

  // JJY Library
  jjy.begin(); // Start JJY Receive
  // jjy.freq(60); // Set frequency 40kHz if SEL pin connected.

}

static auto d = DisplayDigit().setG();
static uint8_t rawBuffer[4] = {0, 0, 0, 0};
uint8_t counter=0;
uint8_t firstreception = 1;

void loop() {
  time_t now = jjy.get_time();
  time_t lastreceived = jjy.getTime();
  tm tm_info;


  localtime_r(&now, &tm_info);
  if(lastreceived != -1){
    led.switchColon();
    char buf[5];
    sprintf(buf, "%d%2d", tm_info.tm_hour, tm_info.tm_min);
    led.display(String(buf));
    firstreception = 0;
  }else{
    if (firstreception == 1){ // Display receiving quality
      counter = (counter + 1) % 4;
      led.colonOff();
      switch(counter){
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
    }else{
      led.colonOn();
      char buf[5];
      sprintf(buf, "%d%2d", tm_info.tm_hour, tm_info.tm_min);
      led.display(String(buf));
    }
  }
  if(tm_info.tm_min == 0 && lastreceived != -1){ // receive from last over an hour.
    jjy.begin();
  } 
  delay(500);
}