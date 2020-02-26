#include <SdFat.h>
//https://github.com/sdesalas/Arduino-Queue.h
#include <queue.h>
#include "config.h"

//The queues hold pointers to the starting addresses of each block of size BLOCK_SIZE.
//This is done to optimize performance and simplify later code.
//These pointers, once initialized, will basically go back and forth between
//the emptyQueue and the writeQueue
Queue<uint8_t*> writeQueue = Queue<uint8_t*>(BLOCK_COUNT);
Queue<uint8_t*> emptyQueue = Queue<uint8_t*>(BLOCK_COUNT);

uint8_t blockBuf[BUFFER_SIZE];

SdExFat sd;
ExFatFile binFile;


void setup() {
  //Populate the empty queue with pointers to each 512 byte-long buffer.
  for(uint32_t i = 0; i < BLOCK_COUNT; i++) {
      uint8_t* bufIndex = &blockBuf[i * BLOCK_SIZE]; //Pointer generated to be at the start of each block.
      emptyQueue.push(bufIndex);
  }
}

static int state = 0;
void loop() {
    switch (state)
    {
    case 0: //Waiting to read.
        /* code */
        break;
    
    default:
        break;
    }
}

void readSensors() {

}
