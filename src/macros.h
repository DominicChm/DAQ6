#include "config.h"

#ifdef DEBUG
    #define debug(s) {(Serial.print(s));}
    #define debugl(s) {(Serial.println(s));}
#else
    #define debug(s) {}
    #define debugl(s) {}
#endif

#define error(s) {debugl("ERROR: " + s)}

#define safeSet(var, val) {cli(); var = val; sei();}

#define hiByte(num) {(uint8_t)(num & 0xFF)}
#define loByte(num) {(uint8_t)((num >> 8) & 0xFF)}

#define SET_STATE_IF(conditional, newstate) {if(conditional){state = newstate; break;}}
#define SET_STATE_EXEC_IF(conditional, newstate, execute) {if(conditional){state = newstate; execute break;}}