#include <Arduino.h>

#define _DEBUG (1)

#ifdef _DEBUG
#define     dprintinit(x)           Serial.begin(x)
#define     dprint(x)               Serial.print(x)
#define     dprintln(x)             Serial.println(x)
#define     hprint(x)               Serial.print(x, HEX)
#define     hprintln(x)             Serial.println(x, HEX)
#else
#define     dprintinit(x)
#define     dprint(x)
#define     dprintln(x)
#define     hprint(x)
#define     hprintln(x)
#endif
