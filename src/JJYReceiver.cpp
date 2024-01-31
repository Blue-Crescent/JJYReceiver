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
  //for(uint8_t index = 0; index < VERIFYLOOP; index++){
    localtime[index] = localtime[index] + 1;
  //}
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

JJYReceiver::delta_tick(){
  uint8_t data = digitalRead(datapin)==HIGH ? 1 : 0;  
  int PM, H, L;
  shift_in(data, sampling, 13);
  sampleindex++;
  if(sampleindex == 100){
    sampleindex = 0;
    clear(sampling,13);
  }
  if(sampleindex == 50){ // 100sampleを毎秒きっかりだと処理時間が厳しいので少し間引く
    clock_tick();
  }else if(sampleindex == 98){ // 100sampleを毎秒きっかりだと処理時間が厳しいので少し間引く
    bitcount++;
    //debug2();
    L = distance(CONST_L , sampling, 13);
    H = distance(CONST_H , sampling, 13);
    PM = distance(CONST_PM , sampling, 13);
    switch(max_of_three(L,H,PM)){
      case 0: // L
        jjypayload[jjystate] <<= 1;
        markercount=0;
        DEBUG_PRINT("L");
      break;
      case 1: // H
        jjypayload[jjystate] <<= 1;
        jjypayload[jjystate] |= 0x1; 
        markercount=0;
        DEBUG_PRINT("H");
      break;
      case 2: // PM
        markercount++;
        jjypayloadlen[jjystate] = bitcount-1;
        bitcount=0;
        if(markercount==2){
          jjystate = JJY_MIN;
          rotateArray((6-jjypayloadcnt),jjypayload,6);
          DEBUG_PRINT("PAYLOADLEN:");
          DEBUG_PRINT((int)jjypayloadcnt);
          debug3();
          jjypayloadcnt=0;
          update_time();
          if(state == SECSYNCED){
            state = PACKETFORMED;
          }else{
            state = SECSYNCED;
          }
        }else{
          // if(jjypos == 5){
           // rotateArray(jjyoffset,jjypayload,6);
          // }
          //jjypos = (jjypos + 1) % 6;
          jjypayloadcnt++;
          jjystate = (jjystate + 1) % 6;
        }
       //   DEBUG_PRINT("PAYLOADLEN:");
       //   for(uint8_t i=0; i<6;i++){
       //     DEBUG_PRINT(jjypayloadlen[i]);
       //     DEBUG_PRINT(" ");
       //   }
       //   DEBUG_PRINTLN("");
        DEBUG_PRINT("P");
      break;
    }
    
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
    DEBUG_PRINT(" M:");
    DEBUG_PRINT((int)markercount);
    DEBUG_PRINT(" BIT:");
    DEBUG_PRINT((int)bitcount);

    debug();

    // DEBUG_PRINT(" JJYPOS:");
    // DEBUG_PRINT((int)jjypos);
    // DEBUG_PRINT(" JJYOFFSET:");
    // DEBUG_PRINT((int)jjyoffset);

    String str = String(ctime(&localtime[0]));
    DEBUG_PRINTLN(str);  // Print current localtime.
    #endif
  }

}

JJYReceiver::update_time(){
 // if(state == PACKETFORMED){
 //  if(jjypayloadlen[JJY_MIN] == 8){
     jjydata[0].bits.min =(uint8_t) 0x00FF & jjypayload[JJY_MIN]; 
 //  }
 //  if(jjypayloadlen[JJY_HOUR] == 9){
     jjydata[0].bits.hour =(uint16_t) 0x006F & jjypayload[JJY_HOUR];
 //  }
 //  if(jjypayloadlen[JJY_DOYH] == 9 && jjypayloadlen[JJY_DOYL] ==9){
     jjydata[0].bits.doyh =(uint16_t) 0x007F & jjypayload[JJY_DOYH]; 
     jjydata[0].bits.doyl =(uint8_t) ((0x01E0 & jjypayload[JJY_DOYL]) >> 5); 
     jjydata[0].bits.parity =(uint8_t) (0x06 & jjypayload[JJY_DOYL]);
 //  }
 //  if(jjypayloadlen[JJY_YEAR] == 9){
     jjydata[0].bits.year =(uint8_t) 0x00FF & jjypayload[JJY_YEAR]; 
 //  }
   settime();
  //}
}
JJYReceiver::jjy_receive(){
  unsigned long time = millis();
  unsigned long window;
  bool data = digitalRead(datapin);  // ピンの状態を読み取る
  if (data == LOW) {
    //if(monitorpin != -1) digitalWrite(monitorpin,LOW);
    window = time - fallingtime[0];
    if(1000 < window){
      sampleindex = 0;
      clear(sampling,13);
    }
    fallingtime[1] = fallingtime[0];
    fallingtime[0] = time;
  }else{
    //if(monitorpin != -1) digitalWrite(monitorpin,HIGH);
      risingtime[1] = risingtime[0];
      risingtime[0] = time;
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
  rcvcnt = 0;
  while(rcvcnt == VERIFYLOOP){
    delay(1000);
  }
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
  return (count % 2 == 0 ? 0 : 1) == expectedParity;
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
      shift_in(bit,jjydata[0].datetime,8);
     }
  }
  settime();

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
    DEBUG_PRINT(jjypayloadcnt);
    DEBUG_PRINT(" :");
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
    case BITSYNC:
      DEBUG_PRINT("BITSYNC");
      break;
    case RECEIVE:
      DEBUG_PRINT("RECEIVE");
      break;
    case DECODE:
      DEBUG_PRINT("DECODE");
      break;
    case TIME:
      DEBUG_PRINT("TIME");
      break;
    case SECSYNCED:
      DEBUG_PRINT("SECSYNCED");
      break;
    case PACKETFORMED:
      DEBUG_PRINT("PACKETFORMED");
      break;
  }
  DEBUG_PRINT(" ");
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
       for(int i=12; i>=0; i--){
         sprintf(buf, "%02X", sampling[i]);
         debugSerial.print(buf);
         if(i==0) debugSerial.print(":");
       }
}
int JJYReceiver::debug3(){
  DEBUG_PRINTLN("PAYLOAD:");
  for(uint8_t i; i<6; i++)
    DEBUG_PRINT(jjypayload[i],HEX);
  DEBUG_PRINTLN(" ");
}
#endif