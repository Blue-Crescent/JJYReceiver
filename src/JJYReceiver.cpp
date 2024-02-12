#include <JJYReceiver.h>

// Reference:
//https://www.nict.go.jp/sts/jjy_signal.html
//https://captain-cocco.com/time-h-c-standart-library/#toc8

/*!
    @brief  Constructor for JJYReceiver
*/
#ifdef DEBUG_BUILD
#ifndef DEBUG_ESP32
  //extern SoftwareSerial Serial;
#endif
#endif

JJYReceiver::JJYReceiver(int pindata,int pinsel,int pinpon) : 
 datapin(pindata), selpin(pinsel), ponpin(pinpon)
 {
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
  timeinfo.tm_sec = (timeinfo.tm_sec + 1) % 60;           // 秒
  globaltime = globaltime + 1;
  //if(state == TIMEVALID) return globaltime;
  //for(uint8_t index = 0; index < VERIFYLOOP; index++){
  //  localtime[index] = localtime[index] + 1;
  //}
  return globaltime;
}

int JJYReceiver::distance(const volatile uint8_t* arr1,volatile uint8_t* arr2, int size) {
    int hammingDistance = 0;
    uint8_t temp;
    for (int i = 0; i < size; i++) {
      temp = ~(arr1[i] ^ arr2[i]);
      for(int j = 0; j < 8; j++){
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
    for (int i = 0; i < 3; i++) {
        int next = (i + 1) % 3;
        if (jjydata[i].bits.year != jjydata[next].bits.year ||
            jjydata[i].bits.doyh != jjydata[next].bits.doyh ||
            jjydata[i].bits.doyl != jjydata[next].bits.doyl ||
            jjydata[i].bits.hour != jjydata[next].bits.hour ||
            jjydata[i].bits.min != ((jjydata[next].bits.min - 1) % 60)) {
            return false;
        }
    }
    last_jjydata = jjydata[0];
    state = TIMEVALID;
    return true;
}

time_t JJYReceiver::get_time() {
  return globaltime;
}
time_t JJYReceiver::get_time(uint8_t index) {
  uint16_t year,yday;
  time_t temptime;
  year = (((jjydata[index].bits.year & 0xf0) >> 4) * 10 + (jjydata[index].bits.year & 0x0f)) + 2000;
  timeinfo.tm_year  = year - 1900; // 年      
  //timeinfo.tm_yday = // Day of the year is not implmented in Arduino time.h
  yday = ((((jjydata[index].bits.doyh >> 5) & 0x0002)) * 100) + (((jjydata[index].bits.doyh & 0x000f)) * 10) + jjydata[index].bits.doyl;
  calculateDate(year, yday ,(uint8_t*) &timeinfo.tm_mon,(uint8_t*) &timeinfo.tm_mday);
  timeinfo.tm_hour  = ((jjydata[index].bits.hour >> 5) & 0x3) * 10 + (jjydata[index].bits.hour & 0x0f) ;         // 時
  timeinfo.tm_min   = ((jjydata[index].bits.min >> 5) & 0x7)  * 10 + (jjydata[index].bits.min & 0x0f) + 1;          // 分
  temptime = mktime(&timeinfo);
  return temptime;
}
time_t JJYReceiver::getTime() {
  uint16_t year,yday;
  switch(state){
   case INIT:
    return -1;
   case RECEIVE:
    return -1;
   case TIMEVALID:
    year = (((last_jjydata.bits.year & 0xf0) >> 4) * 10 + (last_jjydata.bits.year & 0x0f)) + 2000;
    timeinfo.tm_year  = year - 1900; // 年      
    //timeinfo.tm_yday = // Day of the year is not implmented in Arduino time.h
    yday = ((((last_jjydata.bits.doyh >> 5) & 0x0002)) * 100) + (((last_jjydata.bits.doyh & 0x000f)) * 10) + last_jjydata.bits.doyl;
    calculateDate(year, yday ,(uint8_t*) &timeinfo.tm_mon,(uint8_t*) &timeinfo.tm_mday);
    timeinfo.tm_hour  = ((last_jjydata.bits.hour >> 5) & 0x3) * 10 + (last_jjydata.bits.hour & 0x0f) ;         // 時
    timeinfo.tm_min   = ((last_jjydata.bits.min >> 5) & 0x7)  * 10 + (last_jjydata.bits.min & 0x0f) + 1;          // 分
    globaltime = mktime(&timeinfo);
    state = TIMETICK;
    received_time = globaltime;
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
  if(sampleindex == 100){
    sampleindex = 0;
    clear(sampling, N);
  }else if(sampleindex == 90){ // クロックが揺らぐので100sampleしっかりないため少し間引く
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
          }
          #ifdef DEBUG_BUILD
          //debug3();
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
    quality = (uint8_t) (((quality * 100) / (N*8)) - 50) * 2;
    #ifdef DEBUG_BUILD
    debug();
    DEBUG_PRINT(" "); DEBUG_PRINT(L); DEBUG_PRINT(":"); DEBUG_PRINT(H); DEBUG_PRINT(":"); DEBUG_PRINT(PM); DEBUG_PRINT(" Q:") DEBUG_PRINT(quality);
    //DEBUG_PRINT(" "); DEBUG_PRINT(L); DEBUG_PRINT(":"); DEBUG_PRINT(H); DEBUG_PRINT(":"); DEBUG_PRINT(PM); DEBUG_PRINT(" Q:") DEBUG_PRINT(quality);
    #endif
    DEBUG_PRINTLN("");
  }

}

void JJYReceiver::jjy_receive(){
  unsigned long time = millis();
  unsigned long window;
  if(state == TIMEVALID) return;
  bool data = digitalRead(datapin);  // ピンの状態を読み取る
  if (data == LOW) {
    if(monitorpin != -1) digitalWrite(monitorpin,LOW);
    window = time - fallingtime[0];
    if(990 < window){
      sampleindex = 0;
      clear(sampling,N);
    }
    fallingtime[1] = fallingtime[0];
    fallingtime[0] = time;
  }else{
    if(monitorpin != -1) digitalWrite(monitorpin,HIGH);

  }
}
STATE JJYReceiver::status(){
  return state;
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
    DEBUG_PRINTLN(frequency);
    DEBUG_PRINTLN(selpin);
    DEBUG_PRINTLN(ponpin);
  if(ponpin == -1) return true;
  if(powerstate == true){
    digitalWrite(ponpin,LOW);
    if(selpin == -1) return false;
    freq(frequency);
    DEBUG_PRINTLN("POWER ON");
    return true;
  }else{
    digitalWrite(ponpin,HIGH);
    if(selpin == -1) return false;
    digitalWrite(selpin,HIGH);
    DEBUG_PRINTLN("POWER OFF");

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
  state = TIMEVALID;
  power(false);
}

void JJYReceiver::calculateDate(uint16_t year, uint8_t dayOfYear,volatile uint8_t *month,volatile uint8_t *day) {
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
  //(*month)++;
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
   }
  DEBUG_PRINT(" ");
  DEBUG_PRINT((int)jjypayloadlen[jjystate]);
  DEBUG_PRINT("");
}
void JJYReceiver::debug2(){
       char buf[32];
       for(int i = N - 1; i >= 0; i--){
         sprintf(buf, "%02X", sampling[i]);
         Serial.print(buf);
         if(i==0) Serial.print(":");
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
  //DEBUG_PRINT("TESTARRAY:");
  //for(uint8_t i;i<6;i++)
  //  DEBUG_PRINT(testarray[i],HEX);
  DEBUG_PRINTLN("");
  DEBUG_PRINT("PAYLOAD:");
  for(uint8_t i=0; i < 6; i++)
    DEBUG_PRINT(jjypayload[i],HEX);
  DEBUG_PRINTLN("");
}
#endif