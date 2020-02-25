#include "Arduino.h"
#include "config.h"
#include "ChRt.h"
#include "Sensor.h"
#include "LED.h"
#include "SdFat.h"

extern mutex_t bContext_mtx;

extern semaphore_t data_sem;

extern uint8_t BUF0[ABS_BUFFER_SIZE];  
extern uint8_t BUF1[ABS_BUFFER_SIZE];  

extern u_int16_t NUM_SENSORS;
extern Sensor *sensors[SENSOR_ARR_SIZE]; //Can be bigger - acutal num sensors determined during compilation (...?)


extern LED ledTeensy;
extern LED ledLogger;


extern volatile char fileName[8];
extern SD_TYPE sd;
extern FILE_TYPE file;

extern volatile boolean isLogging;

extern volatile uint8_t  writingBuffer;
extern volatile uint8_t  readingBuffer;

extern volatile uint32_t bufferedBytesToWrite;


extern uint8_t* buffers[2];


