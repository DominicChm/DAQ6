#include "ThreadShare.h"


static THD_WORKING_AREA(waWriterThread, 256);

static THD_FUNCTION(WriterThread, arg) {
    debugl("Writer Thread intialized. Starting....");


    while(true) {

        chSemWait(&data_sem); //Wait for data to be availible
        chMtxLock(&bContext_mtx);
        if(bufferedBytesToWrite > 0 && file) {
          debugl("Writing to SD...");
          bool res = file.write(buffers[readingBuffer], bufferedBytesToWrite);
          debugl(res);
          file.sync();

          debug("Wrote ");
          debug(bufferedBytesToWrite);
          debugl(" bytes.")

          bufferedBytesToWrite = 0;
        } else if(bufferedBytesToWrite > 0) {
          bufferedBytesToWrite = 0;
        }
        chMtxUnlock(&bContext_mtx);       
    }
  
}