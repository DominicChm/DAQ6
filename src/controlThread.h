#include <ChRt.h>
#include "ThreadShare.h"
#include "config.h"
#include "DebouncedButton.h"
#include "SDFat.h"
#include "util.h"

static THD_WORKING_AREA(waControlThread, 256);

static THD_FUNCTION(ControlThread, arg) {
    debugl("Control Thread intialized. Starting....");
    
    static DebouncedButton loggerButton = DebouncedButton(PIN_LOGGER_BTN);

    static boolean loggingState = true;

    while(true) {  

        if(loggerButton.isTriggered()) {
            loggingState = !loggingState;
            if(loggingState) { //Start logging.
                Serial.println("TOGGLED ON");
                if(file) {
                    //file.close();
                }

                if(true);
                {
                    Serial.println("Selecting filename");
                    char buf[FILENAME_SIZE];
                    SelectNextFilename((char*) &buf, &sd);

                    Serial.println("Opening");      
                    //file = SD.open((char*) &fileName, FILE_WRITE); //Open the file. Don't ever bother closing it B/C we'll be writing pretty much constantly.           
                }
                
                
                
                
                
                ledTeensy.setState(LED::FAST_BLINK);
                Serial.println(file);
                if(file) {
                    Serial.println("kek");
                }

                chMtxUnlock(&bContext_mtx);
                




            } else { //Stop logging.
                chMtxLock(&bContext_mtx); //Prevents reader thread, and by extention the writer thread, from proceeding.
                Serial.println("TOGGLED OFF");
                //If logging, close the file.
                if(file) {
                    //file.close();
                }
                ledTeensy.setState(LED::OFF);
            }
            
        }
        
        chThdSleepMilliseconds(CONTROLTHD_INTERVAL);
    }
  
}