#include <JJYReceiver.h>
#include <SoftwareSerial.h>
#include <MsTimer2.h>

#define DATA 2

JJYReceiver jjy(DATA); // JJYReceiver lib set up.

void setup() {
  // 10msec Timer for clock ticktock (Mandatory)
  MsTimer2::set(10, ticktock);
  MsTimer2::start();
  
  // DATA pin signal change edge detection. (Mandatory)
  attachInterrupt(digitalPinToInterrupt(DATA), isr_routine, CHANGE);

  // JJY Library
  jjy.begin(); // Start JJY Receive
}

void isr_routine() { // pin change interrupt service routine
  jjy.jjy_receive(); 
}
void ticktock() {  // 10 msec interrupt service routine
  jjy.delta_tick();
}

void loop() {
  time_t now = jjy.get_time();

  delay(10000);
}
