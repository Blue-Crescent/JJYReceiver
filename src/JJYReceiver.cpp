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

// Reference:
//https://www.nict.go.jp/sts/jjy_signal.html

#include <JJYReceiver.h>

/*!
    @brief  Constructor for JJYReceiver
*/
#ifdef DEBUG_BUILD
#ifndef DEBUG_ESP32
  //extern SoftwareSerial Serial;
#endif
#endif

JJYReceiver::JJYReceiver(int pindata,int pinsel,int pinpon) : 
 datapin(pindata), selpin(pinsel), ponpin(pinpon) {
  pinMode(pindata, INPUT);
  pinMode(pinsel, OUTPUT);
  pinMode(pinpon, OUTPUT);
}
JJYReceiver::JJYReceiver(int pindata,int pinpon):
  datapin(pindata),selpin(-1),ponpin(pinpon){
  pinMode(pindata, INPUT);
  pinMode(pinpon, OUTPUT);
}
JJYReceiver::JJYReceiver(int pindata):
  datapin(pindata),selpin(-1),ponpin(-1){
  pinMode(pindata, INPUT);
}

JJYReceiver::~JJYReceiver(){
}

time_t JJYReceiver::clock_tick(){
  globaltime = globaltime + 1;
  return globaltime;
}

int JJYReceiver::distance(const volatile uint8_t* arr1,volatile uint8_t* arr2, int size) {
    int hammingDistance = 0;
    uint8_t temp;
    for (uint8_t i = 0; i < size; i++) {
      temp = ~(arr1[i] ^ arr2[i]);
      for(uint8_t j = 0; j < 8; j++){
        if (((temp >> j) & 0x1) == 1) {
            hammingDistance++;
        }
      }
    }
    return hammingDistance;
}

int JJYReceiver::max_of_three(uint8_t a, uint8_t b, uint8_t c) {
    return (a > b) ? ((a > c) ? 0 : 2) : ((b > c) ? 1 : 2);
}
void JJYReceiver::clear(volatile uint8_t* sampling, int length){
    for (uint8_t i = 0; i < length; i++) {
      sampling[i] = 0;
    }
}


void JJYReceiver::shift_in(uint8_t data,volatile uint8_t* sampling, int length){
  uint8_t carry;
  for (int i = 0; i < length; i++) {
    if(i==0) carry = data;
    uint16_t temp = sampling[i];
    temp = (temp << 1) | carry;
    carry = (temp > 0xFF);
    sampling[i] = temp & 0xFF;
  }
}

bool JJYReceiver::timeCheck(){
    int compare[6][2] = {{0, 1}, {0, 2}, {1, 0}, {1, 2}, {2, 0}, {2, 1}};
    uint8_t min[2];
    uint8_t hour00[2];
    for (int i = 0; i < 6; i++) {
        min[0] = ((jjydata[compare[i][0]].bits.min >> 5) & 0x7)  * 10 + (jjydata[compare[i][0]].bits.min & 0x0f) + 1;
        min[1] = ((jjydata[compare[i][1]].bits.min >> 5) & 0x7)  * 10 + (jjydata[compare[i][1]].bits.min & 0x0f) + 2;
        hour00[0] = ( min[0] == 0 ) ? 1 : 0;
        hour00[1] = ( min[1] == 0 ) ? 1 : 0;
        if (jjydata[compare[i][0]].bits.year == jjydata[compare[i][1]].bits.year && 
            jjydata[compare[i][0]].bits.doyh == jjydata[compare[i][1]].bits.doyh && 
            jjydata[compare[i][0]].bits.doyl == jjydata[compare[i][1]].bits.doyl && 
            (jjydata[compare[i][0]].bits.hour + hour00[0]) == (jjydata[compare[i][1]].bits.hour + hour00[1]) &&
            (abs((min[1] - min[0] + 60) % 60) <= 2))
        {
          tick = 0;
          last_jjydata[0] = (min[1] > min[0]) ? jjydata[compare[i][1]] : jjydata[compare[i][0]];
          state = TIMEVALID;
          power(false);
          return true;
        }
    }
    return false;
}

time_t JJYReceiver::get_time() {
  return globaltime;
}

time_t JJYReceiver::get_time(uint8_t index) {
  return updateTimeInfo(jjydata,index,1);
}
time_t JJYReceiver::getTime() {
  time_t temp_time;
  switch(state){
   case INIT:
    return -1;
   case RECEIVE: // Intermediate update (1st receive update)
    if(timeavailable == -1) return -1;
    temp_time = updateTimeInfo(jjydata,timeavailable,1);
    timeavailable = -1;
    switch(reliability){
      case 1:
        return temp_time;
      break;
    }
    return -1;
   case TIMEVALID:
    globaltime = updateTimeInfo(last_jjydata,0,1);
    state = TIMETICK;
    received_time = globaltime;
    break;
   default:
    return received_time;
  }
  return received_time;
}

void JJYReceiver::delta_tick(){
  uint8_t data, PM, H, L, max;
  tick = (tick+1) % 100;
  if(tick == 0){
    clock_tick();
  }
  if(state == TIMEVALID) return;
  data = digitalRead(datapin)==HIGH ? 1 : 0;  
  shift_in(data, sampling, N);
  sampleindex++;
  if(95 < sampleindex){
    sampleindex = 0;
    clear(sampling, N);
  }else if(sampleindex == 95){
    #ifdef DEBUG_BUILD
    debug2();
    #endif
    L = distance(CONST_L , sampling, N);
    H = distance(CONST_H , sampling, N);
    PM = distance(CONST_PM , sampling, N);
    max = max_of_three(L,H,PM);
    switch(max){
      case 0: // L
        jjypayload[jjystate] <<= 1;
        jjypayloadlen[jjystate]++;
        markercount=0;
        quality = L;
        DEBUG_PRINT("L");
      break;
      case 1: // H
        jjypayload[jjystate] <<= 1;
        jjypayload[jjystate] |= 0x1; 
        jjypayloadlen[jjystate]++;
        markercount=0;
        quality = H;
        DEBUG_PRINT("H");
      break;
      case 2: // PM
        markercount++;
        if(markercount==2){
          rcvcnt = (rcvcnt + 1) % VERIFYLOOP;
          if(settime(rcvcnt)){
            timeCheck();
            timeavailable = rcvcnt;
          }
          #ifdef DEBUG_BUILD
          debug3();
          #endif
          jjypayloadcnt=0;
          jjystate = JJY_MIN;
          for (uint8_t i = 0; i < 6; i++){
            jjypayload[i]=0;
            jjypayloadlen[i]=0;
          }
          DEBUG_PRINT("M");
        }else{
          DEBUG_PRINT("P");
          jjystate = static_cast<JJYSTATE>((jjystate + 1) % 6);
          jjypayloadcnt++;
        }
      quality = PM;
      break;
    }
    quality = (uint8_t) constrain((((quality * 100) / (N*8)) - 50) * 2,0,100);
    #ifdef DEBUG_BUILD
    debug();
    DEBUG_PRINT(" "); DEBUG_PRINT(L); DEBUG_PRINT(":"); DEBUG_PRINT(H); DEBUG_PRINT(":"); DEBUG_PRINT(PM); DEBUG_PRINT(" Q:") DEBUG_PRINT(quality);
    #endif
    DEBUG_PRINTLN("");
  }

}

void JJYReceiver::jjy_receive(){
    if(state == TIMEVALID) return;
  bool data = digitalRead(datapin);  // ピンの状態を読み取る
  if (data == HIGH) {
    if(monitorpin != -1) digitalWrite(monitorpin,HIGH);
    if(sampleindex < 20){
      sampleindex = 0;
      clear(sampling,N);
    }
      }else{
    if(monitorpin != -1) digitalWrite(monitorpin,LOW);
  }
}
uint8_t JJYReceiver::freq(uint8_t freq){
  if(selpin == -1) return -1;
  if(freq == 40){
    digitalWrite(selpin,LOW);
    delay(300);
  }else if(freq == 60){
    digitalWrite(selpin,HIGH);
    delay(300);
  }
  frequency = freq;
  DEBUG_PRINT("FREQ:");
  DEBUG_PRINTLN(frequency);
  return frequency;
}

bool JJYReceiver::power(){
  return (digitalRead(ponpin) == HIGH && digitalRead(selpin) == HIGH) ?  false : true;
}
bool JJYReceiver::power(bool powerstate){
  // PDN1(SEL) PDN2(PON)
  // 0 0 freq2 40kHz
  // 0 1 freq2 (non use)
  // 1 0 freq1 60kHz
  // 1 1 power down
  if(ponpin == -1) return true;
  if(powerstate == true){
    digitalWrite(ponpin,LOW);
    if(selpin == -1) return false;
    freq(frequency);
    DEBUG_PRINTLN("POWER:ON");
    return true;
  }else{
    digitalWrite(ponpin,HIGH);
    if(selpin == -1) return false;
    digitalWrite(selpin,HIGH);
    DEBUG_PRINTLN("POWER:OFF");

    return false;
  }
}

void JJYReceiver::monitor(int pin){
  pinMode(pin, OUTPUT);
  monitorpin = pin;
}

void JJYReceiver::begin(){
  init();
}

void JJYReceiver::stop(){
  power(false);
  state = TIMETICK;
}

//timeinfo.tm_yday = // Day of the year is not implmented in Arduino time.h
void JJYReceiver::calculateDate(uint16_t year, uint16_t dayOfYear,volatile uint8_t *month,volatile uint8_t *day) {
  uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
    // 閏年の場合、2月は29日
    daysInMonth[1] = 29;
  }
  *month = 0;
  while (dayOfYear > daysInMonth[*month]) {
    dayOfYear -= daysInMonth[*month];
    (*month)++;
  }
  *day = dayOfYear;
}

// ***********************************************************************************************
//  DEBUG FUNCTION
// ***********************************************************************************************

#ifdef DEBUG_BUILD
void JJYReceiver::debug(){
    DEBUG_PRINT(" ");
    //DEBUG_PRINT(jjypayloadcnt);
    //DEBUG_PRINT(":");
    switch(jjystate) {
        case JJY_INIT:
            DEBUG_PRINT("INIT");
            break;
        case JJY_MIN:
            DEBUG_PRINT("MIN");
            break;
        case JJY_HOUR:
            DEBUG_PRINT("HOUR");
            break;
        case JJY_DOYH:
            DEBUG_PRINT("DOYH");
            break;
        case JJY_DOYL:
            DEBUG_PRINT("DOYL");
            break;
        case JJY_YEAR:
            DEBUG_PRINT("YEAR");
            break;
        case JJY_WEEK:
            DEBUG_PRINT("WEEK");
            break;
        default:
            DEBUG_PRINT("UNKNOWN");
    }
   DEBUG_PRINT(":");
   switch(state){
   case INIT:
     DEBUG_PRINT("INIT");
     break;
   case RECEIVE:
     DEBUG_PRINT("RECEIVE");
     break;
   case TIMEVALID:
     DEBUG_PRINT("TIMEVALID");
     break;
case TIMETICK:
     DEBUG_PRINT("TIMETICK");
     break;
default:
      break;
   }
  DEBUG_PRINT(" ");
  DEBUG_PRINT((int)jjypayloadlen[jjystate]);
  DEBUG_PRINT("");
}
void JJYReceiver::debug2(){
       char buf[32];
       for(int i = N - 1; i >= 0; i--){
         sprintf(buf, "%02X", sampling[i]);
         DEBUG_PRINT(buf);
         if(i==0) DEBUG_PRINT(":");
       }
}
void JJYReceiver::debug3(){
  DEBUG_PRINTLN("");
  DEBUG_PRINT("PAYLOADLEN:");
  for(uint8_t i=0; i < 6; i++)
    DEBUG_PRINT(jjypayloadlen[i],HEX);
  DEBUG_PRINTLN("");
  DEBUG_PRINT("PAYLOADCNT:");
  DEBUG_PRINTLN((int)jjypayloadcnt);
  DEBUG_PRINTLN("");
  DEBUG_PRINT("PAYLOAD:");
  for(uint8_t i=0; i < 6; i++)
    DEBUG_PRINT(jjypayload[i],HEX);
  DEBUG_PRINTLN("");
}
#endif