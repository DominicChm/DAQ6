#include <Arduino.h>
#include <SdFat.h>
#include "ChRt.h"

#include "ledThread.h"
#include "readerThread.h"
#include "writerThread.h"
#include "controlThread.h"

#include "ThreadShare.h" // <--  ALL VARIABLES ARE STORED HERE FOR WHEN I BREAK THIS INTO MULTIPLE FILES!!!

#include "SensorRotSpeeds.h"
#include "SensorMarker.h"
#include "SensorTime.h"

#include "macros.h"
#include "config.h"
#include "Sensor.h"
#include "util.h"
#include "LED.h"



SensorRotSpeeds* sRotSpeeds;



/**ISRS**/
#ifdef SENSOR_WHEELSPEED
CH_FAST_IRQ_HANDLER(engineSpeedISR)  {sRotSpeeds->calcESpeed();}
CH_FAST_IRQ_HANDLER(rWheelsSpeedISR) {sRotSpeeds->calcRWheels();}
CH_FAST_IRQ_HANDLER(flWheelSpeedISR) {sRotSpeeds->calcFLWheel();}
CH_FAST_IRQ_HANDLER(frWheelSpeedISR) {sRotSpeeds->calcFRWheel();}
#endif



void setupSensors() {
  /*SETUP SENSORS*/
  #ifdef SENSOR_TIME
    sensors[NUM_SENSORS++] = new SensorTime(); 
  #endif
  #ifdef SENSOR_MARKER
    sensors[NUM_SENSORS++] = new SensorMarker(PIN_MARKER_BTN); 
  #endif
  #ifdef SENSOR_WHEELSPEED
    //Hold a reference in a var to call interrupts on the sensor obj.
    sensors[NUM_SENSORS++] = sRotSpeeds = new SensorRotSpeeds(); 

    //Credit to Rahul. yoink.

    attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_ENG),  engineSpeedISR, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_RGO), rWheelsSpeedISR, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_WFL), flWheelSpeedISR, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_WFR), frWheelSpeedISR, RISING);
  #endif
}





void chSetup() {

  setupSensors();
  
  /*SETUP AND START THREADS*/
  chThdCreateStatic(waReaderThread, sizeof(waReaderThread),
      READER_PRIORITY, ReaderThread, NULL);

  chThdCreateStatic(waWriterThread, sizeof(waWriterThread),
      WRITER_PRIORITY, WriterThread, NULL);
      
  chThdCreateStatic(waControlThread, sizeof(waControlThread),
      CONTRL_PRIORITY, ControlThread, NULL);

  chThdCreateStatic(waLEDThread, sizeof(waLEDThread),
      LEDDRV_PRIORITY, LEDThread, NULL);


}

void setup() {
  /*SETUP SERIAL*/
  Serial.begin(250000);
  while (!Serial) {;} 
  debugl("Starting DAQ...");
  
  pinMode(PIN_MARKER_BTN, INPUT_PULLUP);
  
  /*Sensor Setup*/
  


  /*SETUP SD*/ //Will eventually move file creation into its own function.
  if (sd.begin(254)) {
    SelectNextFilename((char*) &fileName, &sd);                 
    if(!file.open((char*) &fileName, O_RDWR | O_CREAT)){ //Open the file. Don't ever bother closing it B/C we'll be writing pretty much constantly.
      debugl("File open err");
    } else {
      debugl("Filed opened successfully");
    }
    ledTeensy.setState(LED::FAST_BLINK);

  } else {
    debugl("SD FAILED TO INITIALIZE - Running in SD-less mode!");;
    sd.initErrorHalt(&Serial);
    ledTeensy.setState(LED::FAST_BLINK);
  }

  
  chBegin(chSetup);
}

/*=*=*=*SECTION LOOP*=*=*=*/
void loop() {
  
}

