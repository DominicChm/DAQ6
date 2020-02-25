#include <ChRt.h>
#include "macros.h"
#include "ThreadShare.h"
#include "config.h"

static THD_WORKING_AREA(waLEDThread, 128);

static THD_FUNCTION(LEDThread, arg) {
    
    /*LEDThread init*/
    
    debugl("LED Thread intialized. Starting...");

    
    while(true) {
        ledTeensy.tick();
        //ledLogger.tick();
        chThdSleepMilliseconds(LEDTHD_INTERVAL); 
    }
}