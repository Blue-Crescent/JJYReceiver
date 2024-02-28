
#include <LiquidCrystal.h>
#include <JJYReceiver.h>

#define DATA 39
#define PON 33
#define SEL 25

hw_timer_t * timer = NULL;

JJYReceiver jjy(DATA,SEL,PON); // JJYReceiver lib set up.
LiquidCrystal lcd = LiquidCrystal(16,17,4,0,2,15); // RS, Enable, D4, D5, D6, D7

void IRAM_ATTR ticktock() {
  jjy.delta_tick();
}

void isr_routine() { // pin change interrupt service routine
  jjy.jjy_receive(); 
}

void setup()
{
    
    // 10msec Timer for clock ticktock (Mandatory)
    
    timer = timerBegin(0, 80, true);  // タイマー0, 分周比80（これにより1カウントが1マイクロ秒になる）
    timerAttachInterrupt(timer, &ticktock, true);  // 割り込み関数をアタッチ
    timerAlarmWrite(timer, 10000, true);  // 10ミリ秒のタイマー（10,000マイクロ秒）
    timerAlarmEnable(timer);  // タイマーを有効にする
    attachInterrupt(digitalPinToInterrupt(DATA), isr_routine, CHANGE);


    jjy.freq(40); // Carrier frequency setting. Default:40
    jjy.begin(); // Start JJY Receive
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0,0);

}

void loop()
{
  time_t now = jjy.get_time();
  time_t lastreceived = jjy.getTime();
  tm tm_info;
  tm tm_lastinfo;

  localtime_r(&now, &tm_info);
  localtime_r(&lastreceived, &tm_lastinfo);
  
  lcd.clear();
  lcd.setCursor(0,0);
  char buf1[16];
  if(lastreceived==-1){
    lcd.print("Receiving.. Q:");
    lcd.print(jjy.quality);
    lcd.setCursor(0,1);
    strftime(buf1, sizeof(buf1), "%H:%M:%S", &tm_info);
    lcd.print(buf1);
  }else{
    if(tm_info.tm_sec % 10 < 5){
      strftime(buf1, sizeof(buf1), "Last:%H:%M", &tm_lastinfo);
    }else{
      strftime(buf1, sizeof(buf1), "%Y/%m/%d(%a)", &tm_info);
    }
    lcd.print(buf1);
    lcd.setCursor(0,1);
    char buf3[16];
    strftime(buf3, sizeof(buf3), "%H:%M:%S", &tm_info);
    lcd.print(buf3);
  }

  delay(100);
  if(tm_info.tm_min == 0 && lastreceived != -1){ // Receive start on the hour 
    jjy.begin();
  } 
}
