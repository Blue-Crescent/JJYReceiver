#include "lgt_LowPower.h"
#include <SoftwareSerial.h>
#include <MsTimer2.h>


#include <JJYReceiver.h>

#define DATA 2
#define PON 8
#define SEL 12

#define MONITORPIN LED_BUILTIN

SoftwareSerial debugSerial(7, 6); // RX, TX


JJYReceiver jjy;

void setup() {
  // For LGT8f328 board setting.
  pinMode(10, INPUT);  pinMode(11, INPUT); // DATA
  pinMode(9, INPUT); // PON
  // Debug print
  debugSerial.begin(115200);

  // Time for clock ticktock
  MsTimer2::set(10, ticktock);
  MsTimer2::start();
  
  
  // JJYReceiver lib setup.
  digitalWrite(MONITORPIN,HIGH);
  delay(1000);
  jjy.begin(DATA,SEL,PON);
  attachInterrupt(digitalPinToInterrupt(DATA), isr_routine, CHANGE);

  jjy.monitor(MONITORPIN);
  jjy.freq(40); // Carrier frequency setting. Default:40
  
  debugSerial.println("JJY Initialized.");
}

void isr_routine() { // pin change interrupt service routine
  jjy.jjy_receive(); 
}
void ticktock() {  // 10 msec interrupt service routine
  jjy.delta_tick();
}
hay
void loop() {
  delay(10000);
  time_t now = jjy.getTime();
  if(now != -1){
    String str = String(ctime(&now));
    debugSerial.println(str);  // Print current localtime.

    // After time received, it can release Timer and isr routine.
    detachInterrupt(digitalPinToInterrupt(DATA));

    // If RTC used, it can stop Timer after RTC time updated.
    // MsTimer2::stop();
  }else{
    String str = "Receiving:";
    debugSerial.print(str);
    str = String(ctime(&now));
    debugSerial.println(str);  // Print current localtime.
  }
  
  
  
}
