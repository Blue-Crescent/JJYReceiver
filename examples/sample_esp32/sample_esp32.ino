
#include <JJYReceiver.h>

#define DATA 39
#define PON 33
#define SEL 25

JJYReceiver jjy(DATA,SEL,PON); // JJYReceiver lib set up.

Ticker timer;

void IRAM_ATTR ticktock() {
  jjy.delta_tick();
}

void isr_routine() { // pin change interrupt service routine
  jjy.jjy_receive(); 
}

void setup() {
  // Debug print
  Serial.begin(115200);
  
  // 10msec Timer for clock ticktock (Mandatory)
  timer.attach_ms(10, ticktock);

  // DATA pin signal change edge detection. (Mandatory)
  attachInterrupt(digitalPinToInterrupt(DATA), isr_routine, CHANGE);
  
  // // JJY Library
  jjy.begin(); // Start JJY Receive
  jjy.freq(40); // Carrier frequency setting. Default:40
  
  Serial.println("JJY Initialized.");
}


void loop() {
  time_t now = jjy.get_time();
  time_t lastreceived = jjy.getTime();

  if(lastreceived != -1){
    String str = String(ctime(&now));str.trim();
    Serial.println(" "+str); // Print current date time.

    Serial.print(" Last received:");    
    str = String(ctime(&lastreceived)); str.trim();
    Serial.println(str);  // Print last received time
  }else{
    String str0 = "Receiving... Quality:";
    String str1 = String(jjy.quality);
    Serial.println(str0 + str1);
    
    String str = String(ctime(&now)); str.trim();
    Serial.println(" "+str);

    // Belows are for debug. Accessing intermediate JJY time.
    String local[3];
    time_t temp = jjy.get_time(0);
    local[0] = String(ctime(&temp)); local[0].trim(); // debug print of 1st received time 
    Serial.println(" "+local[0]);

    temp = jjy.get_time(1);
    local[1] = String(ctime(&temp)); local[1].trim(); // debug print of 2nd received time 
    Serial.println(" "+local[1]);

    temp = jjy.get_time(2);
    local[2] = String(ctime(&temp)); local[2].trim(); // debug print of 3rd received time
    Serial.println(" "+local[2]);
  }

  if((now - lastreceived) > 3600 && lastreceived != -1){ // receive from last over an hour.
    jjy.begin();
  } 
  delay(1000);
}
