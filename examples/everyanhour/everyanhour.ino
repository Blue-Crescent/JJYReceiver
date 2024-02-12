#include <Arduino.h>
#include <JJYReceiver.h>
//#include <SoftwareSerial.h>
#include <MsTimer2.h>

#define DATA 2
#define PON 8
#define SEL 12

SoftwareSerial debugSerial(7, 6); //For debug RX, TX

JJYReceiver jjy(DATA,SEL,PON); // JJYReceiver lib set up.

void setup() {
  // Only For LGT8f328 board setting. This can be ignored for other board.
  pinMode(10, INPUT);  pinMode(11, INPUT); // DATA
  pinMode(9, INPUT); // PON

  // Debug print
  Serial.begin(115200);

  // 10msec Timer for clock ticktock (Mandatory)
  MsTimer2::set(10, ticktock);
  MsTimer2::start();
  // DATA pin signal change edge detection. (Mandatory)
  attachInterrupt(digitalPinToInterrupt(DATA), isr_routine, CHANGE);

  // JJY Library
  jjy.begin(); // Start JJY Receive
  jjy.freq(40); // Carrier frequency setting. Default:40
  
  Serial.println("JJY Initialized.");

  while(jjy.getTime() == -1){
    delay(1000); // wait until time available. 受信するまで待ち
  }
}

void isr_routine() { // pin change interrupt service routine
  jjy.jjy_receive(); 
}
void ticktock() {  // 10 msec interrupt service routine
  jjy.delta_tick();
}

void loop() {
  time_t now = jjy.get_time();
  tm tm;

  localtime_r(&now, &tm);
  String str1 = String(tm.tm_hour);
  String str2 = String(tm.tm_min);
  String str3 = String(tm.tm_sec);
  Serial.println(str1 + ':' + str2 + ' ' + str3);  // Print current date time.

  if (tm.tm_min == 0) { // run every an hour. 1時間ごとに受信
    jjy.begin();  
  }
  
  delay(1000);
}
