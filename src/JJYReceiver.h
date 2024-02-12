#ifndef JJYReceiver_h
#define JJYReceiver_h

#define VERIFYLOOP 3

#include <time.h>
#include <Arduino.h>
#include <stdint.h>

//#define DEBUG_BUILD
//#define DEBUG_ESP32

#ifdef DEBUG_BUILD
# define DEBUG_PRINT(...)  Serial.print(__VA_ARGS__);
# define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__);

#ifndef DEBUG_ESP32
// For LGT8F328P
//#include <SoftwareSerial.h>
//extern SoftwareSerial debugSerial;
# define DEBUG_PRINT(...)  debugSerial.print(__VA_ARGS__);
# define DEBUG_PRINTLN(...) debugSerial.println(__VA_ARGS__);
#endif
//HardwareSerial& Serial = Serial0;
#else
# define DEBUG_PRINT(fmt,...)
# define DEBUG_PRINTLN(fmt,...)
#endif

const int N = 12;
enum STATE {INIT,RECEIVE,TIMEVALID,TIMETICK};
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
    volatile uint8_t jjypayloadlen[6] = {0,0,0,0,0,0};
    JJYData jjydata[VERIFYLOOP];
    JJYData last_jjydata;
    volatile enum STATE state = INIT;
    volatile enum JJYSTATE jjystate = JJY_INIT;
    volatile uint8_t rcvcnt = 0;
    volatile unsigned long fallingtime[2];
    volatile const int8_t datapin,selpin,ponpin;
    volatile int8_t monitorpin = -1;
    volatile uint8_t frequency = 0;
    volatile uint8_t markercount = 0;
    volatile uint8_t reliability = 0;
    volatile uint8_t quality = 0;

    volatile uint8_t tick = 0;
    volatile uint16_t jjypayload[6]; // 9bits bit data between marker
    volatile int8_t jjypayloadcnt = -2;

    volatile uint8_t sampleindex = 0;
    volatile uint8_t sampling [N];
    volatile const uint8_t CONST_PM [N] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0x00,0x00,0x00};
    volatile const uint8_t CONST_H [N]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    volatile const uint8_t CONST_L [N]  = {0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    volatile time_t globaltime = 0;
    volatile time_t received_time = -1;
    struct tm timeinfo;
    

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
    STATE status();
    uint8_t freq(uint8_t freq);
    void monitor(int pin);
    void calculateDate(uint16_t year, uint8_t dayOfYear,volatile uint8_t *month,volatile uint8_t *day);
    int distance(const volatile uint8_t* arr1,volatile uint8_t* arr2, int size);
    int max_of_three(uint8_t a, uint8_t b, uint8_t c);
    bool calculateParity(uint8_t value, uint8_t bitLength, uint8_t expectedParity);
    bool timeCheck();
    time_t getTime();
    time_t get_time();
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
      jjydata[index].bits.year =(uint8_t) 0x00FF & jjypayload[JJY_YEAR]; 
      jjydata[index].bits.doyh =(uint16_t) 0x007F & jjypayload[JJY_DOYH]; 
      jjydata[index].bits.doyl =(uint8_t) ((0x01E0 & jjypayload[JJY_DOYL]) >> 5); 
      jjydata[index].bits.hour =(uint16_t) 0x006F & jjypayload[JJY_HOUR];
      jjydata[index].bits.min =(uint8_t) 0x00FF & jjypayload[JJY_MIN]; 
      timeinfo.tm_sec   = 1;           // 秒
      return true;
     }
    void init(){
      state = RECEIVE;
      clear(sampling,N);
      jjydata[0].bits.hour = 25;
      jjydata[1].bits.hour = 26;
      jjydata[2].bits.hour = 27;
      power(true);
    }
    bool lencheck(volatile uint8_t* arr) {
        if (arr[0] != 8) {
            return false;
        }
        for (int i = 1; i < 5; i++) { // except WEEK
            if (arr[i] != 9) {
                return false;
            }
        }
        return true;
    }

};
#endif


