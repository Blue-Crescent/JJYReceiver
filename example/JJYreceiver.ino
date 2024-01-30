#include "lgt_LowPower.h"
#include <SoftwareSerial.h>
#include <MsTimer2.h>


#include "JJYReceiver.h"

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

  jjy.receive_nonblock(); // Start JJY analyzation.
  
  debugSerial.println("JJY Initialized.");
}

void isr_routine() { 
  jjy.jjy_receive(); 
}
void ticktock() { 
  jjy.delta_tick();
}


char buf[32];

void loop() {
  // jjy.datetest();
  // jjy.printJJYData(jjy.jjydata[0]);


  // String str = String(ctime(&jjy.localtime[0]));
  // debugSerial.println(str);  // Print current localtime.
  // str = String(ctime(&jjy.localtime[1]));
  // debugSerial.println(str);  // Print current localtime.
  // str = String(ctime(&jjy.localtime[2]));
  // debugSerial.println(str);  // Print current localtime.

  // String str2 = String(" - Receive state : ");
  // String str3 = String((int)jjy.rcvcnt);
  // debugSerial.print(str2+str3);  // Print current receive count.

  // String str4 = String(" - Frequency : ");
  // String str5 = String(jjy.freq());
  // debugSerial.print(str4+str5);  // Print current power.

  // String str6 = String(" - Pon state : ");
  // String str7 = jjy.power() ? "on" : "off";
  // debugSerial.println(str6+str7);  // Print current power.
  // debugSerial.print((int)jjy.bitcnt);
  // debugSerial.print(" | ");
  // debugSerial.print((int)jjy.rcvcnt);
  // debugSerial.print(" | ");
  // String str1 = String(jjy.pulsewidth);
  // debugSerial.print(jjy.fallingtime);
  // debugSerial.print(" - ");
  // debugSerial.print(jjy.risingtime);
  // debugSerial.print(" = ");
  
  // debugSerial.print(str1);
  // debugSerial.print(" = ");
  // debugSerial.print((jjy.risingtime - jjy.prev_risingtime));
  // debugSerial.print(" ^ ");
  // debugSerial.print((jjy.fallingtime - jjy.prev_fallingtime));

  // debugSerial.print(" : ");
  // debugSerial.print((char)jjy.debug);
  // debugSerial.println(" : ");
  


  
  //sampling =  (sampling << 0x1) | (state ? 1:0);
  //sprintf(buf, "%d", sampling);
  // sprintf(buf, "%lu ", jjy.edge[0]);
  // debugSerial.print(buf);
  // sprintf(buf, "%lu ", jjy.edge[1]);
  // debugSerial.print(buf);
  // sprintf(buf, "%lu ", jjy.edge[2]);
  // debugSerial.println(buf);


  // sprintf(buf, "%d ", jjy.diff[1] );
  // debugSerial.print(buf);
  // sprintf(buf, "%d -> ", jjy.diff[0] );
  // debugSerial.print(buf);
  // sprintf(buf, "%d", jjy.syncmillis );
  // debugSerial.println(buf);
  
  
  


 

  // char buf[17];
  // sprintf(buf, "%016llX", jjy.jjydata[0].datetime);
  //debugSerial.println(buf);
  delay(1000);

  // String str8 = String(" - Raw data : ");
  // String str9 = uintToStr(jjy.jjydata[0].datetime);
  // String str10 = String(" - ");
  // String str9 = String((unsigned long long)jjy.jjydata[1].datetime);
  // String str12 = String(" - ");
  // String str9 = String((unsigned long long)jjy.jjydata[2].datetime);
  // debugSerial.println(str8+str9+str10+str11+str12+str13);  // Print current power.


  // struct tm *time_info;
  // time_info = gmtime(&jjy.localtime);
  // if(time_info->tm_min == 0){ // 毎正時に受信を開始
  //   jjy.receive_nonblock();
  // }
}
