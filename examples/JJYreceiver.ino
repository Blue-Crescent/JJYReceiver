#include <JJYReceiver.h>
#include <SoftwareSerial.h>
#include <MsTimer2.h>

#define DATA 2
#define PON 8
#define SEL 12

#define MONITORPIN LED_BUILTIN

SoftwareSerial debugSerial(7, 6); // RX, TX

JJYReceiver jjy(DATA,SEL,PON);

void setup() {
  // For LGT8f328 board setting.
  pinMode(10, INPUT);  pinMode(11, INPUT); // DATA
  pinMode(9, INPUT); // PON
  // Debug print
  debugSerial.begin(115200);

  // 10msec Timer for clock ticktock
  MsTimer2::set(10, ticktock);
  MsTimer2::start();
  
  // JJYReceiver lib sep.
  digitalWrite(MONITORPIN,HIGH);
  delay(1000);
  jjy.begin();
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

void loop() {
  delay(10000);
  
  time_t now = jjy.get_time();
  time_t lastreceived = jjy.getTime();

  if(lastreceived != -1){
    String str = String(ctime(&now));
    debugSerial.print(str);  // Print current date time.

    debugSerial.print(" Last received:");    
    str = String(ctime(&lastreceived));
    debugSerial.println(str);  // Print last received time
  }else{
    String str = "Receiving:";
    debugSerial.print(str);
    str = String(ctime(&now));
    debugSerial.println(str);
  } 
}
