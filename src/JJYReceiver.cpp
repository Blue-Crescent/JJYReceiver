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
}

JJYReceiver::~JJYReceiver(){
}

time_t JJYReceiver::clock_tick(){
  globaltime = globaltime + 1;
  if(state == TIMEVALID) return;
  for(uint8_t index = 0; index < VERIFYLOOP; index++){
    localtime[index] = localtime[index] + 1;
  }
  return globaltime;
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

int JJYReceiver::rotateArray16(int8_t shift, uint16_t* array, uint8_t size) {
  for (uint8_t i = 0; i < size; i++) {
    array[i] = (array[i] + shift + size) % size;
  }
}
int JJYReceiver::rotateArray8(int8_t shift, uint8_t* array, uint8_t size) {
  for (uint8_t i = 0; i < size; i++) {
    array[i] = (array[i] + shift + size) % size;
  }
}
time_t JJYReceiver::getTime() {
    time_t diff1 = labs(localtime[0] - localtime[1]);
    time_t diff2 = labs(localtime[1] - localtime[2]);
    time_t diff3 = labs(localtime[2] - localtime[0]);
    if( diff1 < 2){
      power(false);
      if(state != TIMEVALID) globaltime = localtime[1];
      state = TIMEVALID;
      return localtime[1];
    }else if(diff2 < 2){
      power(false);
      if(state != TIMEVALID) globaltime = localtime[2];
      state = TIMEVALID;
      return localtime[2];
    }else if(diff3 < 2){
      power(false);
      if(state != TIMEVALID) globaltime = localtime[0];
      state = TIMEVALID;
      return localtime[0];
    }
    //DEBUG_PRINT(diff1);
    //DEBUG_PRINT(" ");
    //DEBUG_PRINT(diff2);
    //DEBUG_PRINT(" ");
    //DEBUG_PRINTLN(diff3);
    return -1;
}
time_t JJYReceiver::get_time() {
  return globaltime;
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
    clear(sampling,N);
  }else if(sampleindex == 95){ // クロックが揺らぐので100sampleしっかりないため少し間引く
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
          if(settime(rcvcnt)){
            rcvcnt = (rcvcnt + 1) % VERIFYLOOP;
            getTime();
          }//else{
            //rotateArray16((jjypayloadcnt),testarray,6);
            //rotateArray16((jjypayloadcnt),jjypayload,6);
            //rotateArray8((jjypayloadcnt),jjypayloadlen,6);
          //}
          #ifdef DEBUG_BUILD
          debug3();
          #endif
          jjypayloadcnt=0;
          jjystate = JJY_MIN;
          for (uint8_t i = 0; i < 6; i++){
            jjypayload[i]=0;
            jjypayloadlen[i]=0;
          }
          DEBUG_PRINTLN("M");
        }else{
          DEBUG_PRINT("P");
          jjystate = (jjystate + 1) % 6;
          jjypayloadcnt++;
        }
      quality = PM;
      break;
    }
    quality = (quality * 100) / 96;
    #ifdef DEBUG_BUILD
    debug();
    debug4();
    DEBUG_PRINT(" "); DEBUG_PRINT(L); DEBUG_PRINT(":"); DEBUG_PRINT(H); DEBUG_PRINT(":"); DEBUG_PRINT(PM); DEBUG_PRINT(" Q:") DEBUG_PRINT(quality);
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
  if(ponpin == -1) return true;
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
  init();
}
JJYReceiver::begin(int pindata,int pinsel){
  pinMode(pindata, INPUT);
  pinMode(pinsel, OUTPUT);
  datapin = pindata;
  selpin = pinsel;
  init();
}
JJYReceiver::begin(int pindata){
  pinMode(pindata, INPUT);
  datapin = pindata;
  init();
}
JJYReceiver::stop(){

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

// ***********************************************************************************************
//  DEBUG FUNCTION
// ***********************************************************************************************

#ifdef DEBUG_BUILD
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
   DEBUG_PRINT(":");
   switch(state){
   case INIT:
     DEBUG_PRINT("INIT");
     break;
   case RECEIVE:
     DEBUG_PRINT("RECEIVE");
     break;
   case DATAVALID:
     DEBUG_PRINT("DATAVALID");
     break;
   case TIMEVALID:
     DEBUG_PRINT("TIMEVALID");
     break;
   case STOP:
     DEBUG_PRINT("STOP");
     break;
   }
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
  DEBUG_PRINTLN("");
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
  DEBUG_PRINTLN("");
}
int JJYReceiver::debug4(){
    DEBUG_PRINT(" ");  // Print current localtime.
    String str = String(ctime(&localtime[0]));
    String marker;
    marker = (rcvcnt == 0) ? "*" : " ";
    DEBUG_PRINT(marker + str);  // Print current localtime.
    DEBUG_PRINT(" ");  // Print current localtime.
    str = String(ctime(&localtime[1]));
    marker = (rcvcnt == 1) ? "*" : " ";
    DEBUG_PRINT(marker + str);  // Print current localtime.
    DEBUG_PRINT(" ");  // Print current localtime.
    str = String(ctime(&localtime[2]));
    marker = (rcvcnt == 2) ? "*" : " ";
    DEBUG_PRINT(marker + str);  // Print current localtime.
    DEBUG_PRINT(" =>");  // Print current localtime.
    str = String(ctime(&globaltime));
    DEBUG_PRINT(str);  // Print current localtime.
}
#endif