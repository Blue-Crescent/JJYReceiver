
#include <LiquidCrystal.h>
#include <JJYReceiver.h>
#include "esp_timer.h"

#define DATA 39
#define PON 33
#define SEL 25

JJYReceiver jjy(DATA,SEL,PON); // JJYReceiver lib set up.
LiquidCrystal lcd = LiquidCrystal(16,17,4,0,2,15); // RS, Enable, D4, D5, D6, D7

void ticktock(void* arg) {
  jjy.delta_tick();
}

void isr_routine() { // pin change interrupt service routine
  jjy.jjy_receive(); 
}



esp_timer_handle_t timer;

void ticktock(void* arg);   // 旧ISRと同じ名前でOK
void isr_routine();         // DATAピン割り込み

void setup()
{
    // ===== 10msec Timer for clock ticktock (Mandatory) =====
  Serial.begin(115200);
    // esp_timer の設定
  esp_timer_create_args_t timer_args = {
      .callback = &ticktock,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "ticktock",
      .skip_unhandled_events = false
  };


    // タイマー作成
    esp_timer_create(&timer_args, &timer);

    // 10ms = 10000us 周期で実行
    esp_timer_start_periodic(timer, 10000);


    // ===== DATA ピンの立ち上がり/立ち下がり割り込み =====
    attachInterrupt(digitalPinToInterrupt(DATA), isr_routine, CHANGE);


    // ===== JJY 受信開始 =====
    
    // jjy.freq(60);
    jjy.begin();

    // ===== LCD 初期化 =====
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
}


void loop()
{
  time_t now = jjy.get_time();
  time_t lastreceived = jjy.getTime();
  tm tm_info;
  tm tm_lastinfo;

  
  localtime_r(&now, &tm_info);
  localtime_r(&lastreceived, &tm_lastinfo);

  lcd.setCursor(0,0);
  lcd.clear();
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
  if(tm_info.tm_min % 30 == 0 && lastreceived != -1){ // receive from last over an hour.
    jjy.begin();
  } 
}