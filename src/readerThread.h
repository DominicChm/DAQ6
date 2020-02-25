#include <ChRt.h>
#include "macros.h"
#include "ThreadShare.h"
#include "util.h"


/*STATE CONSTS*/
static const int READER_STATE_WAITING = 0;
static const int READER_STATE_READING = 1;
static const int READER_STATE_BSWTICH = 2;
static const int READER_STATE_RESET = 3;

/*******************************READER THREAD****************************/
static THD_WORKING_AREA(waReaderThread, 256);

static THD_FUNCTION(ReaderThread, arg) {
  /*THREAD INIT*/
  static volatile uint32_t writingHead = 0;
  static volatile systime_t time = chVTGetSystemTimeX();
  static volatile int state = 0;

  debugl("Reader Thread intialized. Starting...");

  while(true) {
    switch(state) {
      case READER_STATE_WAITING: { /*STATE_WAITING*/
        //debugl("w");
        time += TIME_MS2I(SAMPLE_INTERVAL);
        //debugl(time);
        
        if(time < chVTGetSystemTime()) time = chVTGetSystemTime() + SAMPLE_INTERVAL; //Handles the case where the system has fallen behind.

        chThdSleepUntil(time); //All waiting is handled by ChibiOS. Thread will resume here after wait MS
        
        state = READER_STATE_READING; 
        break;}


      case READER_STATE_READING: { //Buffers one packet. Then checks size and switches buffers if needed.
          writingHead += BufferPacket(
              &buffers[writingBuffer][writingHead],
              sensors,
              NUM_SENSORS);

          if( writingHead >= BUFFER_SIZE ) state = READER_STATE_BSWTICH;
          else state = READER_STATE_WAITING;


        break;}


      case READER_STATE_BSWTICH: { //Switches buffer contexts.
        //Lock mutex while we're switching. 
        //Makes sure that there is no writing happening and that no writing will happen while we're switching.

        
        chMtxLock(&bContext_mtx); 
        debugl("Switching buffers...");
        readingBuffer =  writingBuffer;
        writingBuffer = !writingBuffer;

        bufferedBytesToWrite = writingHead;

        writingHead = 0;
        chMtxUnlock(&bContext_mtx);
        chSemSignal(&data_sem);

        state = READER_STATE_WAITING; //Always go back to idle state.
        break;}

    }
  }
}
