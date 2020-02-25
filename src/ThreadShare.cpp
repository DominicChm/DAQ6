#include "ThreadShare.h"
#include "Arduino.h"
#include "config.h"
#include "ChRt.h"
#include "Sensor.h"
#include "LED.h"
#include "SdFat.h"


MUTEX_DECL(bContext_mtx);

SEMAPHORE_DECL(data_sem, 0);



uint8_t BUF0[ABS_BUFFER_SIZE];  
uint8_t BUF1[ABS_BUFFER_SIZE];  

uint8_t* buffers[2] = {
    BUF0,
    BUF1
};

uint16_t NUM_SENSORS = 0;

Sensor *sensors[SENSOR_ARR_SIZE];


LED ledTeensy(PIN_STATUS_LED);
LED ledLogger(PIN_LOGGER_LED);


char volatile fileName[8];

SD_TYPE sd;
FILE_TYPE file;

volatile boolean isLogging = true;


uint8_t volatile writingBuffer  = 0;
uint8_t volatile readingBuffer  = 0;

volatile uint32_t bufferedBytesToWrite   = 0;

