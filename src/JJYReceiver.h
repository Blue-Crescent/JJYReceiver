// MIT License
// 
// Copyright (c) 2024 Blue-Crescent <ick40195+github@gmail.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef JJYReceiver_h
#define JJYReceiver_h

#define VERIFYLOOP 3
#define FREQSWITCHTHRESHOLD 60
#define FREQSWITCHTHRESHOLD2 900

#include <time.h>
#include <Arduino.h>
#include <stdint.h>

// #define SWAPFREQ
// #define DEBUG_BUILD
// #define DEBUG_ESP32

// Following checking features can be applied for JJY only
#define PARITYCHK
#define WEEKDAYCHK

#ifdef DEBUG_BUILD
#define DEBUG_PRINT(...)  Serial.print(__VA_ARGS__);
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__);

#ifndef DEBUG_ESP32
// For LGT8F328P
//#include <SoftwareSerial.h>
//extern SoftwareSerial debugSerial;
#define DEBUG_PRINT(...)  debugSerial.print(__VA_ARGS__);
#define DEBUG_PRINTLN(...) debugSerial.println(__VA_ARGS__);
#endif
//HardwareSerial& Serial = Serial0;
#else
#define DEBUG_PRINT(fmt,...)
#define DEBUG_PRINTLN(fmt,...)
#endif

const int N = 12;
typedef union {
    uint8_t datetime[8];
    struct {
        uint8_t dummy : 4;
        uint8_t P0 : 1;
        uint8_t space3 : 4;
        uint8_t leap : 2;
        uint8_t weekday : 3;
        uint8_t P5 : 1;
        uint8_t year : 8;
        uint8_t su2 : 1 ;
        uint8_t P4 : 1;
        uint8_t su1 : 1 ;
        uint8_t parity : 2 ;
        uint8_t space2 : 2 ;
        uint8_t doyl : 4 ;
        uint8_t P3 : 1 ;
        uint16_t doyh : 9 ;
        uint8_t space1 : 2 ;
        uint8_t P2 : 1 ;
        uint16_t hour : 9 ;
        uint8_t P1 : 1 ;
        uint8_t min : 8 ;
        uint8_t M : 1 ;
    } bits;
} JJYData;

class JJYReceiver {
    enum STATE {INIT,RECEIVE,TIMEVALID,TIMETICK};
    enum JJYSTATE {JJY_MIN=0,JJY_HOUR=1,JJY_DOYH=2,JJY_DOYL=3,JJY_YEAR=4,JJY_WEEK=5};
    enum AUTOFREQ {FREQ_AUTO=1, FREQ_MANUAL=0};
  
  public:
    volatile uint8_t jjypayloadlen[6] = {0,0,0,0,0,0};
    JJYData jjydata[VERIFYLOOP];
    JJYData last_jjydata[1];
    volatile enum STATE state = INIT;
    volatile enum JJYSTATE jjystate = JJY_MIN;
    volatile uint8_t rcvcnt = 0;
    volatile const int8_t datapin,selpin,ponpin;
    volatile int8_t monitorpin = -1;
    volatile uint8_t frequency = 40;
    volatile uint8_t markercount = 0;
    volatile uint8_t reliability = 0;
    volatile uint8_t quality = 0;

    volatile uint8_t tick = 0;
    volatile uint16_t jjypayload[6]; // 9bits bit data between marker

    volatile uint8_t sampleindex = 0;
    volatile uint8_t sampling [N];
    volatile int8_t timeavailable = -1;
    volatile const uint8_t CONST_PM [N] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00};
    volatile const uint8_t CONST_H [N]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0x07,0x00,0x00,0x00,0x00,0x00,0x00};
    volatile const uint8_t CONST_L [N]  = {0xFF,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    volatile time_t globaltime = 0;
    volatile time_t received_time = -1;
    volatile time_t start_time = -1; // FREQ_AUTO時は周波数変更時点から
    struct tm timeinfo;
    volatile uint8_t autofreq = FREQ_AUTO;
    uint16_t year, yday;
    uint8_t jjy_weekday;
    uint8_t calc_weekday;
    long diff;

    JJYReceiver(int pindata);
    JJYReceiver(int pindata,int pinpon);
    JJYReceiver(int pindata,int pinpon, int pinsel);
    ~JJYReceiver();
    void jjy_receive();
    time_t clock_tick();
    void delta_tick();
    void shift_in(uint8_t data,volatile uint8_t* sampling, int length);
    void clear(volatile uint8_t* sampling, int length);
    void begin();
    void stop();
    bool power(bool powerstate);
    bool power();
    uint8_t freq(uint8_t freq);
    void monitor(int pin);
    void calculateDate(uint16_t year, uint16_t dayOfYear,volatile uint8_t *month,volatile uint8_t *day);
    int distance(const volatile uint8_t* arr1,volatile uint8_t* arr2, int size);
    int max_of_three(uint8_t a, uint8_t b, uint8_t c);
    bool timeCheck();
    time_t getTime();
    time_t get_time();
    long set_time(time_t newtime);
    time_t get_time(uint8_t index);

    #ifdef DEBUG_BUILD
    void debug();
    void debug2();
    void debug3();
    void debug4();
    #endif

  private:
    bool settime(uint8_t index){
      if(lencheck(jjypayloadlen) == false){
         return false;
      }
      #ifdef PARITYCHK 
      uint8_t PA_2 = (jjypayload[JJY_DOYL] & 0x0002) >> 1;
      uint8_t PA_1 = (jjypayload[JJY_DOYL] & 0x0004) >> 2;
      if(!calculate_even_parity(jjypayload[JJY_MIN],0xEF,PA_2)){
         return false;
      }
      if(!calculate_even_parity(jjypayload[JJY_HOUR],0x6F,PA_1)){
         return false;
      }
      #endif
      jjydata[index].bits.year =(uint8_t) 0x00FF & jjypayload[JJY_YEAR]; 
      jjydata[index].bits.doyh =(uint16_t) 0x007F & jjypayload[JJY_DOYH]; 
      jjydata[index].bits.doyl =(uint8_t) ((0x01E0 & jjypayload[JJY_DOYL]) >> 5); 
      jjydata[index].bits.hour =(uint16_t) 0x006F & jjypayload[JJY_HOUR];
      jjydata[index].bits.min =(uint8_t) 0x00FF & jjypayload[JJY_MIN]; 
      #ifdef WEEKDAYCHK
      jjy_weekday = (uint8_t) ((0x01C0 & jjypayload[JJY_WEEK]) >> 6);
      year = getyear(jjydata,index);
      yday = getyday(jjydata,index);
      calc_weekday = get_weekday(year, yday);
      if(calc_weekday != jjy_weekday){
        return false;
      }
      #endif
      timeinfo.tm_sec   = 1;           // 秒
      return true;
    }
    uint16_t getyday(JJYData* jjydata, int8_t index){
        uint16_t yday;
        yday = ((((jjydata[index].bits.doyh >> 5) & 0x0003)) * 100) + (((jjydata[index].bits.doyh & 0x000f)) * 10) + jjydata[index].bits.doyl;
        return yday;
    }
    uint16_t getyear(JJYData* jjydata, int8_t index){
        uint16_t year;
        year = (((jjydata[index].bits.year & 0x00F0) >> 4) * 10 + (jjydata[index].bits.year & 0x000f)) + 2000;
        return year;
    }
    time_t updateTimeInfo(JJYData* jjydata, int8_t index, int8_t offset) {
        uint16_t year, yday;
        year = getyear(jjydata,index);
        timeinfo.tm_year  = year - 1900; // 年      
        yday = getyday(jjydata,index);
        calculateDate(year, yday ,(uint8_t*) &timeinfo.tm_mon,(uint8_t*) &timeinfo.tm_mday);
        timeinfo.tm_hour  = ((jjydata[index].bits.hour >> 5) & 0x3) * 10 + (jjydata[index].bits.hour & 0x0f) ;         // 時
        timeinfo.tm_min   = ((jjydata[index].bits.min >> 5) & 0x7)  * 10 + (jjydata[index].bits.min & 0x0f) + offset;          // 分
        return mktime(&timeinfo);
    }
    void init(){
      clear(sampling,N);
      jjydata[0].bits.hour = 25;
      jjydata[1].bits.hour = 26;
      jjydata[2].bits.hour = 27;
      power(true);
      state = RECEIVE;
      start_time = globaltime;
    }
    bool lencheck(volatile uint8_t* arr) {
        if (arr[0] != 8) {
            return false;
        }
        for (int i = 1; i < 6; i++) {
            if (arr[i] != 9) {
                return false;
            }
        }
        return true;
    }
    #ifdef PARITYCHK
    // 偶数パリティを計算する関数
    bool calculate_even_parity(uint8_t data, uint8_t mask,uint8_t parity_bit) {
        uint8_t parity = parity_bit;

        // 指定されたビット数に基づいてパリティを計算
        for (uint8_t i = 0; i < 8; i++) {
            // mask の i ビット目が1なら、そのビットをパリティ計算に含める
            if ((mask >> i) & 1) {
                parity ^= (data >> i) & 1;
            }
        }

        return parity == 0;
    }
    #endif
    #ifdef WEEKDAYCHK
    uint8_t get_weekday(uint16_t year, uint16_t yday)
    {
        // 1月1日の曜日を求める（Zellerの公式）
        uint16_t zyear = year - 1;
        uint8_t j = zyear / 100;
        uint8_t k = zyear - (j * 100);
        uint8_t base_weekday = (uint8_t)((43 + k + (k>>2) + (j>>2) - (j<<1)) % 7); // Sunday = 0

        // yday を加算して曜日を求める
        uint8_t weekday = (uint8_t)((base_weekday + yday - 1) % 7);
        return weekday;
    }
    #endif
    void clearpayload(){
        for (uint8_t i = 0; i < 6; i++){
            jjypayload[i]=0;
            jjypayloadlen[i]=0;
        }
    }
    void autoselectfreq(JJYSTATE jjystate){
        uint8_t count = (uint8_t) jjypayloadlen[jjystate];
        if(count > FREQSWITCHTHRESHOLD && autofreq==FREQ_AUTO){
            frequency == 40 ?  setfreq(60) : setfreq(40);
            DEBUG_PRINT("FREQ SWITCHED1: ")
            DEBUG_PRINTLN(frequency)
            clearpayload();
        }
        if( (globaltime - start_time) > FREQSWITCHTHRESHOLD2 && autofreq==FREQ_AUTO){
            frequency == 40 ?  setfreq(60) : setfreq(40);
            start_time = globaltime;
            DEBUG_PRINT("FREQ SWITCHED2: ")
            DEBUG_PRINTLN(frequency)
            clearpayload();
        }
    }
    void setfreq(uint8_t freq){
        if(freq==40){
            #ifdef SWAPFREQ
            digitalWrite(selpin,HIGH);
            #else
            digitalWrite(selpin,LOW);
            #endif
        }else{
            #ifdef SWAPFREQ
            digitalWrite(selpin,LOW);
            #else
            digitalWrite(selpin,HIGH);
            #endif
        }
        frequency = freq;
    }
};
#endif


