#ifndef JJYReceiver_h
#define JJYReceiver_h

#define VERIFYLOOP 3

#include <time.h>
#include <Arduino.h>
#include <stdint.h>

#define DEBUG_BUILD

#ifdef DEBUG_BUILD
#include <SoftwareSerial.h>
extern SoftwareSerial debugSerial;
# define DEBUG_PRINT(...)  debugSerial.print(__VA_ARGS__);
# define DEBUG_PRINTLN(...) debugSerial.println(__VA_ARGS__);
#else
# define DEBUG_PRINT(fmt,...)
# define DEBUG_PRINTLN(fmt,...)
#endif

#define N 13
//enum STATE {INIT,BITSYNC,RECEIVE,DECODE,TIME,PACKETFORMED,SECSYNCED};
enum STATE {INIT,RECEIVE,DATAVALID,TIMEVALID,STOP};
enum JJYSTATE {JJY_INIT=-1,JJY_MIN=0,JJY_HOUR=1,JJY_DOYH=2,JJY_DOYL=3,JJY_YEAR=4,JJY_WEEK=5};
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
  
	public:
    JJYData jjydata[VERIFYLOOP];
    volatile uint8_t rcvcnt = 0;
    volatile enum STATE state = INIT;
    volatile unsigned long risingtime[2], fallingtime[2];
    int datapin,ponpin,selpin;
    // int agcpin;
    volatile uint8_t markercount = 0;

    volatile enum JJYSTATE jjystate = JJY_INIT;
    volatile uint8_t jjyoffset = 0;
    volatile uint8_t jjypos = 0;
    volatile uint8_t tick = 0;
    volatile uint16_t jjypayload[6]; // 8bits bit data between marker
    volatile uint8_t jjypayloadlen[6] = {0,0,0,0,0,0}; // 
    volatile int8_t jjypayloadcnt = -2;

    volatile uint8_t sampleindex = 0;
    volatile uint8_t sampling [N] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    volatile uint8_t CONST_PM [N] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0x00,0x00,0x00,0x00};
    volatile uint8_t CONST_H [N]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    volatile uint8_t CONST_L [N]  = {0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    int monitorpin = -1;
    volatile time_t localtime[3] = {-100,-200,-300};
    volatile time_t globaltime;
    volatile struct tm timeinfo;
    
  #ifdef DEBUG_BUILD
    char buf[32];
  #endif

		JJYReceiver();
    ~JJYReceiver();
    int jjy_receive();
    int clock_tick();
    int delta_tick();
    int shift_in(uint8_t data, uint8_t* sampling, int length);
    int clear(uint8_t* sampling, int length);
    int begin(int datapin);
    int begin(int datapin,int pon);
    int begin(int datapin,int pon,int sel);
    int stop();
    // int begin(int datapin,int pon,int sel,int agcpin);
    // int agc(bool agc);
    int power(bool power);
    int power();
    int status();
    int freq(int freq);
    int freq();
    int monitor(int monitor);
    int rotateArray(int8_t shift, uint16_t* array, uint8_t size);
    int calculateDate(uint16_t year, uint8_t dayOfYear, uint8_t *month, uint8_t *day);
    int distance(uint8_t* arr1, uint8_t* arr2, int size);
    int max_of_three(uint8_t a, uint8_t b, uint8_t c);
    bool calculateParity(uint8_t value, uint8_t bitLength, uint8_t expectedParity);
    time_t getTime();
    #ifdef DEBUG_BUILD
    int debug();
    int debug2();
    int debug3();
    int debug4();
    #endif

  private:
    void settime(uint8_t index){
      jjydata[index].bits.year =(uint8_t) 0x00FF & jjypayload[JJY_YEAR]; 
      jjydata[index].bits.doyh =(uint16_t) 0x007F & jjypayload[JJY_DOYH]; 
      jjydata[index].bits.doyl =(uint8_t) ((0x01E0 & jjypayload[JJY_DOYL]) >> 5); 
      jjydata[index].bits.hour =(uint16_t) 0x006F & jjypayload[JJY_HOUR];
      jjydata[index].bits.min =(uint8_t) 0x00FF & jjypayload[JJY_MIN]; 

      uint16_t year = (((jjydata[index].bits.year & 0xf0) >> 4) * 10 + (jjydata[index].bits.year & 0x0f)) + 2000;
      timeinfo.tm_year  = year - 1900; // 年      
      //timeinfo.tm_yday = // Day of the year is not implmented in Arduino time.h
      uint16_t yday = ((((jjydata[index].bits.doyh >> 5) & 0x0002)) * 100) + (((jjydata[index].bits.doyh & 0x000f)) * 10) + jjydata[index].bits.doyl;
      calculateDate(year, yday ,&timeinfo.tm_mon, &timeinfo.tm_mday);
      timeinfo.tm_hour  = ((jjydata[index].bits.hour >> 5) & 0x3) * 10 + (jjydata[index].bits.hour & 0x0f) ;         // 時
      timeinfo.tm_min   = ((jjydata[index].bits.min >> 5) & 0x7)  * 10 + (jjydata[index].bits.min & 0x0f) + 1;          // 分
      timeinfo.tm_sec   = 1;           // 秒

      localtime[index]= mktime(&timeinfo);
     }
    void init(){
      state = RECEIVE;
      for(uint8_t index = 0; index < VERIFYLOOP; index++){
        localtime[index] = index * -100;
      }
      power(true);
    }

};
#endif


