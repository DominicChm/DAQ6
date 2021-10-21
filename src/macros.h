#include "config.h"

#ifdef DEBUG
    #define debug(s) {(Serial.print(s));}
    #define debugl(s) {(Serial.println(s));}
#else
    #define debug(s) {}
    #define debugl(s) {}
#endif


#ifdef PRINT_SENSORS
    #define sensorPrintl(s) {debugl(s);}
    #define sensorPrint(s) {debug(s);}
#else
    #define sensorPrintl(s) {}
    #define sensorPrintl(s) {}
#endif


#define error(s) {debugl("ERROR: " + s)}

#define safeSet(var, val) {cli(); var = val; sei();}

#define hiByte(num) {(uint8_t)((num >> 8) & 0xFF)}
#define loByte(num) {(uint8_t)(num & 0xFF)}

