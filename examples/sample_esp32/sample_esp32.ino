
#include <JJYReceiver.h>

#define DATA 39
#define PON 33
#define SEL 25

JJYReceiver jjy(DATA,SEL,PON); // JJYReceiver lib set up.

hw_timer_t * timer = NULL;

void IRAM_ATTR ticktock() {
  jjy.delta_tick();
}

void isr_routine() { // pin change interrupt service routine
  jjy.jjy_receive(); 
}

void setup() {
  // Debug print
  Serial.begin(115200);
  delay(1000);

  // 10msec Timer for clock ticktock (Mandatory)
  timer = timerBegin(0, 80, true);  // タイマー0, 分周比80（これにより1カウントが1マイクロ秒になる）
  timerAttachInterrupt(timer, &ticktock, true);  // 割り込み関数をアタッチ
  timerAlarmWrite(timer, 10000, true);  // 10ミリ秒のタイマー（10,000マイクロ秒）
  timerAlarmEnable(timer);  // タイマーを有効にする

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
  tm tm_info;

  if(lastreceived != -1){
    String str = String(ctime(&now));
    Serial.println(" "+str); // Print current date time.

    Serial.print(" Last received:");    
    str = String(ctime(&lastreceived));
    Serial.print(str);  // Print last received time
  }else{
    String str0 = "Receiving quality:";
    String str1 = String(jjy.quality);
    Serial.print(str0 + str1);
    
    String str = String(ctime(&now)); str.trim();
    Serial.print(" "+str);
    
    for(int i=0; i<6; i++)
      str = String(jjy.jjypayloadlen[i]);

    str = String(ctime((const time_t*)&jjy.localtime[0])); str.trim();
    Serial.print(" "+str);
    str = String(ctime((const time_t*)&jjy.localtime[1])); str.trim();
    Serial.print(" "+str);
    str = String(ctime((const time_t*)&jjy.localtime[2])); str.trim();
    Serial.println(" "+str);
  }

  if((now - lastreceived) > 3600 && lastreceived != -1){ // receive from last over an hour.
    jjy.begin();
  } 
  delay(10000);
}