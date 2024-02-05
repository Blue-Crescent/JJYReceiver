#include <JJYReceiver.h>

//#include <sys/time.h>

// Reference:
//https://www.nict.go.jp/sts/jjy_signal.html
//https://captain-cocco.com/time-h-c-standart-library/#toc8

/*!
    @brief  Constructor for JJYReceiver
*/
#ifdef DEBUG_BUILD
  
  extern SoftwareSerial debugSerial;
#endif


  uint16_t testarray[6] = {0,1,2,3,4,5};
JJYReceiver::JJYReceiver(){
  for(int index = 0; index < VERIFYLOOP; index++){
    for(int j=0;j <8; j++)  
      jjydata[index].datetime[j] = 0;
  }
}

JJYReceiver::~JJYReceiver(){
}

JJYReceiver::clock_tick(){
  uint8_t index = 0;
  for(uint8_t index = 0; index < VERIFYLOOP; index++){
    localtime[index] = localtime[index] + 1;
  }
}


int JJYReceiver::distance(uint8_t* arr1, uint8_t* arr2, int size) {
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
JJYReceiver::clear(uint8_t* sampling, int length){
    for (uint8_t i = 0; i < length; i++) {
      sampling[i] = 0;
    }
}

int JJYReceiver::shift_in(uint8_t data, uint8_t* sampling, int length){
  uint8_t carry;
  for (int i = 0; i < length; i++) {
    if(i==0) carry = data;
    uint16_t temp = sampling[i];
    temp = (temp << 1) | carry;
    carry = (temp > 0xFF);
    sampling[i] = temp & 0xFF;
  }
}

int JJYReceiver::rotateArray(int8_t shift, uint16_t* array, uint8_t size) {
  for (uint8_t i = 0; i < size; i++) {
    array[i] = (array[i] + shift + size) % size;
  }
}
JJYReceiver::next_payload(){
}

JJYReceiver::delta_tick(){
  uint8_t data = digitalRead(datapin)==HIGH ? 1 : 0;  
  uint8_t PM, H, L;
  shift_in(data, sampling, N);
  sampleindex++;
  if(sampleindex == 100){
    sampleindex = 0;
    clear(sampling,N);
  }
  if(sampleindex == 0){
    clock_tick();
  }else if(sampleindex == 90){ // クロックが揺らぐので100sampleしっかりないため少し間引く
    debug2();
    L = distance(CONST_L , sampling, N);
    H = distance(CONST_H , sampling, N);
    PM = distance(CONST_PM , sampling, N);
    switch(max_of_three(L,H,PM)){
      case 0: // L
        jjypayload[jjystate] <<= 1;
        jjypayloadlen[jjystate]++;
        markercount=0;
        DEBUG_PRINT("L");
      break;
      case 1: // H
        jjypayload[jjystate] <<= 1;
        jjypayload[jjystate] |= 0x1; 
        jjypayloadlen[jjystate]++;
        markercount=0;
        DEBUG_PRINT("H");
      break;
      case 2: // PM
        markercount++;
        if(markercount==2){
          timeinfo.tm_sec = 1;
          localtime[rcvcnt]= mktime(&timeinfo);
          if(state != PACKETFORMED){
            rotateArray((jjypayloadcnt),jjypayload,6);
            rotateArray((jjypayloadcnt),testarray,6);
            for(uint8_t i; i<6; i++){
              update_time(jjystate);
            }
          }
          debug3();
          
          if(state == SECSYNCED){
            state = PACKETFORMED;
          }else{
            state = SECSYNCED;
          }
          rcvcnt = (rcvcnt + 1) % VERIFYLOOP;
          jjypayloadcnt=0;
          jjystate = JJY_MIN;
          for (uint8_t i = 0; i < 6; i++){
            jjypayload[i]=0;
            jjypayloadlen[i]=0;
          }
          DEBUG_PRINTLN("M");
        }else{
          DEBUG_PRINT("P");
          update_time(jjystate);
          jjystate = (jjystate + 1) % 6;
          jjypayloadcnt++;
        }
      break;
    }
    debug();
    #ifdef DEBUG_BUILD
    // DEBUG_PRINT(" JJYDATA:");
    // for(int i=7; i>=0; i--){
    //   sprintf(buf, "%02X", jjydata[0].datetime[i]);
    //   DEBUG_PRINT(buf);
    // }
    // DEBUG_PRINT(" jjypayload:");
    // for(int i=5; i>=0; i--){
    //   sprintf(buf, "%02X", jjypayload[i]);
    //   DEBUG_PRINT(buf);
    // }
    //DEBUG_PRINT(" M:");
    //DEBUG_PRINT((int)markercount);
    //DEBUG_PRINT(" BIT:");
    //DEBUG_PRINT((int)bitcount);

    //debug();

    // DEBUG_PRINT(" JJYPOS:");
    // DEBUG_PRINT((int)jjypos);
    // DEBUG_PRINT(" JJYOFFSET:");
    // DEBUG_PRINT((int)jjyoffset);
    DEBUG_PRINTLN("");
    #endif
  }

}

JJYReceiver::update_time(int jjystate){
  uint8_t errflag = 0x0;
  //if(state == PACKETFORMED){
  switch(jjystate){
    case JJY_MIN:
      if( jjypayloadlen[JJY_MIN] == 8){
       //uint16_t min;
       jjydata[rcvcnt].bits.min =(uint8_t) 0x00FF & jjypayload[JJY_MIN]; 
       //min = ((jjydata[rcvcnt].bits.min >> 5) & 0x7)  * 10 + (jjydata[rcvcnt].bits.min & 0x0F) + 1; // 分
       //timeinfo.tm_min = min;
       //localtime[rcvcnt]= mktime(&timeinfo);
       //DEBUG_PRINT("min:");
       //DEBUG_PRINT(min);
       settime(rcvcnt);
      }
      break;
    case JJY_HOUR:
      if(jjypayloadlen[JJY_HOUR] == 9){
       //uint16_t hour;
       jjydata[rcvcnt].bits.hour =(uint16_t) 0x006F & jjypayload[JJY_HOUR];
       //hour = ((jjydata[rcvcnt].bits.hour >> 5) & 0x3) * 10 + (jjydata[rcvcnt].bits.hour & 0x0f) ;         // 時
       //timeinfo.tm_hour  = hour;
       //localtime[rcvcnt]= mktime(&timeinfo);
       //DEBUG_PRINT("hour:");
       //DEBUG_PRINT(hour);
       settime(rcvcnt);
      }
      break;
    case JJY_DOYH:
      if(jjypayloadlen[JJY_DOYH] == 9 && jjypayloadlen[JJY_DOYL] ==9){
       jjydata[rcvcnt].bits.doyh =(uint16_t) 0x007F & jjypayload[JJY_DOYH]; 
       jjydata[rcvcnt].bits.doyl =(uint8_t) ((0x01E0 & jjypayload[JJY_DOYL]) >> 5); 
       //errflag |= calculateParity(jjydata[rcvcnt].bits.min, 8,(0x01 & (jjypayload[JJY_DOYL]>>1)));
       //errflag |= calculateParity(jjydata[rcvcnt].bits.hour,8,(0x01 & (jjypayload[JJY_DOYL]>>2)));
       //settime(rcvcnt);
      }
      break;
    case JJY_YEAR:
      if(jjypayloadlen[JJY_YEAR] == 9){
       jjydata[rcvcnt].bits.year =(uint8_t) 0x00FF & jjypayload[JJY_YEAR]; 
       //uint16_t year = (((jjydata[rcvcnt].bits.year & 0xf0) >> 4) * 10 + (jjydata[rcvcnt].bits.year & 0x0f)) + 2000;
       //timeinfo.tm_year  = year - 1900; // 年      
       //uint16_t yday = ((((jjydata[rcvcnt].bits.doyh >> 5) & 0x0002)) * 100) + (((jjydata[rcvcnt].bits.doyh & 0x000f)) * 10) + jjydata[rcvcnt].bits.doyl;
       //calculateDate(year, yday ,&timeinfo.tm_mon, &timeinfo.tm_mday);
       //DEBUG_PRINT("year:");
       //DEBUG_PRINT(timeinfo.tm_year);
       //DEBUG_PRINT("yday:");
       //DEBUG_PRINT(yday);
       settime(rcvcnt);
      }
      break;
    //case JJY_WEEK:
    //  if(jjypayloadlen[JJY_WEEK] == 9){
    //   //jjydata[rcvcnt].bits.weekday =(uint8_t) (0x01C0 & jjypayload[JJY_WEEK] >> 6); 
    //   //if(timeinfo.tm_wday != jjydata[rcvcnt].bits.weekday ) errflag |= 0x1;
    //  }
    //  break;
  }
    //localtime[rcvcnt]= mktime(&timeinfo);
    DEBUG_PRINTLN("");  // Print current localtime.
    String str = String(ctime(&localtime[0]));
    String marker;
    marker = (rcvcnt == 0) ? "*" : " ";
    DEBUG_PRINTLN(marker + str);  // Print current localtime.
    str = String(ctime(&localtime[1]));
    marker = (rcvcnt == 1) ? "*" : " ";
    DEBUG_PRINTLN(marker + str);  // Print current localtime.
    str = String(ctime(&localtime[2]));
    marker = (rcvcnt == 2) ? "*" : " ";
    DEBUG_PRINTLN(marker + str);  // Print current localtime.
  //}
}
JJYReceiver::jjy_receive(){
  unsigned long time = millis();
  unsigned long window;
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
  // if(rcvcnt == VERIFYLOOP){
  //   power(false);
  // }
}
JJYReceiver::status(){
  return state;
}
JJYReceiver::freq(){
  return (digitalRead(selpin) == LOW) ?  40 : 60;
}
JJYReceiver::freq(int freq){
  if(freq == 40){
    digitalWrite(selpin,LOW);
    delay(300);
    return 40;
  }else if(freq == 60){
    digitalWrite(selpin,HIGH);
    delay(300);
    return 60;
  }
}

JJYReceiver::power(){
  return (digitalRead(ponpin) == LOW) ?  true : false;
}
JJYReceiver::power(bool power){
  if(power == true){
    digitalWrite(ponpin,LOW);
    delay(300);
    return true;
  }else{
    digitalWrite(ponpin,HIGH);
    return false;
  }
}

JJYReceiver::monitor(int monitorpin){
  pinMode(monitorpin, OUTPUT);
  JJYReceiver::monitorpin = monitorpin;
}

JJYReceiver::begin(int pindata,int pinsel,int pinpon){
  
  pinMode(pindata, INPUT);
  pinMode(pinsel, OUTPUT);
  pinMode(pinpon, OUTPUT);
  datapin = pindata;
  selpin = pinsel;
  ponpin = pinpon;
  state = BITSYNC;
}
JJYReceiver::begin(int pindata,int pinsel){
  pinMode(pindata, INPUT);
  pinMode(pinsel, OUTPUT);
  datapin = pindata;
  selpin = pinsel;
  state = BITSYNC;
}
JJYReceiver::begin(int pindata){
  pinMode(pindata, INPUT);
  datapin = pindata;
  state = BITSYNC;
}
//JJYReceiver::receive_nonblock(){
//  power(true);
//  rcvcnt = 0;
//}
JJYReceiver::receive(){
  power(true);
  state = RECEIVE;
}
int JJYReceiver::calculateDate(uint16_t year, uint8_t dayOfYear, uint8_t *month, uint8_t *day) {
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
bool JJYReceiver::calculateParity(uint8_t value, uint8_t bitLength, uint8_t expectedParity) {
  byte count = 0;
  for (uint8_t i = 0; i < bitLength; i++) {
    if (value & (1 << i)) {
      count++;
    }
  }
  if((count % 2 == 0 ? 0 : 1) == expectedParity){ // Parity OK
    return 0;
  }else{// Parity Error
    return 1;
  }
}
#ifdef DEBUG_BUILD
JJYReceiver::datetest(){
  // 2016/6/10 Fri 17:15
  uint8_t test[8] = {0x12,0x84,0xE1,0x30,0x84,0x0B,0x28,0x00};
   //0001 0010 1000 0100 1110
   //0001 0011 0000 1000 0100 
   //0000 1011 0010 1000 0000 
  
  for(int j=0; j<8; j++){
     for(int i=7; i>=0; i--){
      uint16_t bit=0;
      bit = (test[j] >> i) & 0x1 ;
      //sprintf(buf, "%X", bit );
      debugSerial.print(bit);
      shift_in(bit,jjydata[rcvcnt].datetime,8);
     }
  }
  settime(rcvcnt);

}
JJYReceiver::printJJYData(const JJYData& data) {
  debugSerial.println("datetime:");
  for (int i = 0; i < 8; i++) {
    debugSerial.print(data.datetime[i], HEX);
    debugSerial.print(" ");
  }
  debugSerial.println();

  debugSerial.println("bits:");
  debugSerial.print("M: "); debugSerial.println(data.bits.M, HEX);
  debugSerial.print("min: "); debugSerial.println(data.bits.min, HEX);
  debugSerial.print("P1: "); debugSerial.println(data.bits.P1, HEX);
  debugSerial.print("hour: "); debugSerial.println(data.bits.hour, HEX);
  debugSerial.print("P2: "); debugSerial.println(data.bits.P2, HEX);
  debugSerial.print("space1: "); debugSerial.println(data.bits.space1, HEX);
  debugSerial.print("doyh: "); debugSerial.println(data.bits.doyh, HEX);
  debugSerial.print("P3: "); debugSerial.println(data.bits.P3, HEX);
  debugSerial.print("doyl: "); debugSerial.println(data.bits.doyl, HEX);
  debugSerial.print("space2: "); debugSerial.println(data.bits.space2, HEX);
  debugSerial.print("parity: "); debugSerial.println(data.bits.parity, HEX);
  debugSerial.print("su1: "); debugSerial.println(data.bits.su1, HEX);
  debugSerial.print("P4: "); debugSerial.println(data.bits.P4, HEX);
  debugSerial.print("su2: "); debugSerial.println(data.bits.su2, HEX);
  debugSerial.print("year: "); debugSerial.println(data.bits.year, HEX);
  debugSerial.print("P5: "); debugSerial.println(data.bits.P5, HEX);
  debugSerial.print("weekday: "); debugSerial.println(data.bits.weekday, HEX);
  debugSerial.print("leap: "); debugSerial.println(data.bits.leap, HEX);
  debugSerial.print("space3: "); debugSerial.println(data.bits.space3, HEX);
  debugSerial.print("P0: "); debugSerial.println(data.bits.P0, HEX);
  
  debugSerial.print("dummy: "); debugSerial.println(data.bits.dummy, HEX);
}

JJYReceiver::debug(){
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
   //DEBUG_PRINT(":");
   //switch(state){
   // case INIT:
   //   DEBUG_PRINT("INIT");
   //   break;
   // case BITSYNC:
   //   DEBUG_PRINT("BITSYNC");
   //   break;
   // case RECEIVE:
   //   DEBUG_PRINT("RECEIVE");
   //   break;
   // case DECODE:
   //   DEBUG_PRINT("DECODE");
   //   break;
   // case TIME:
   //   DEBUG_PRINT("TIME");
   //   break;
   // case SECSYNCED:
   //   DEBUG_PRINT("SECSYNCED");
   //   break;
   // case PACKETFORMED:
   //   DEBUG_PRINT("PACKETFORMED");
   //   break;
   // }
  DEBUG_PRINT(" ");
  DEBUG_PRINT((int)jjypayloadlen[jjystate]);
  DEBUG_PRINT("");
}

// JJYReceiver::agc(bool activate){
//   if(activate == true){
//     digitalWrite(agcpin,LOW);
//     return 1;
//   }else{
//     digitalWrite(agcpin,HIGH);
//     return 0;
//   }
// }
// JJYReceiver::begin(int datapin,int sel,int pon,int agc){
//   pinMode(datapin, INPUT);
//   pinMode(sel, OUTPUT);
//   pinMode(pon, OUTPUT);
//   pinMode(agc, OUTPUT);
//   JJYReceiver::datapin = datapin;
// }
int JJYReceiver::debug2(){
       char buf[32];
       for(int i=N-1; i>=0; i--){
         sprintf(buf, "%02X", sampling[i]);
         debugSerial.print(buf);
         if(i==0) debugSerial.print(":");
       }
}
int JJYReceiver::debug3(){
  DEBUG_PRINT("PAYLOADLEN:");
  for(uint8_t i;i<6;i++)
    DEBUG_PRINT(jjypayloadlen[i],HEX);
  DEBUG_PRINTLN("");
  DEBUG_PRINT("PAYLOADCNT:");
  DEBUG_PRINTLN((int)jjypayloadcnt);
  DEBUG_PRINT("TESTARRAY:");
  for(uint8_t i;i<6;i++)
    DEBUG_PRINT(testarray[i],HEX);
  DEBUG_PRINTLN("");
  DEBUG_PRINT("PAYLOAD:");
  for(uint8_t i; i<6; i++)
    DEBUG_PRINT(jjypayload[i],HEX);
  DEBUG_PRINTLN(" ");
}
int JJYReceiver::debug4(){
}
#endif