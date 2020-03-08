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

#define hiByte(num) {(uint8_t)(num & 0xFF)}
#define loByte(num) {(uint8_t)((num >> 8) & 0xFF)}

#define SET_STATE(newstate) {lastState = state; state=newstate;}
#define SET_STATE_IF(conditional, newstate) {if(conditional){SET_STATE(newstate); break;}}
#define SET_STATE_EXEC_IF(conditional, newstate, execute) {if(conditional){SET_STATE(newstate); execute; break;}}
#define ON_STATE_ENTER(execute) {if(state != lastState) {execute; SET_STATE(state);} }
#define STATE_DEFINITIONS static int state = 0; static int lastState = 0;