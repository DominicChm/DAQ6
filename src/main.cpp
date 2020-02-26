#include <SdFat.h>
//https://github.com/sdesalas/Arduino-Queue.h
#include <queue.h>
#include "config.h"
#include "util.h"

/*Sensors*/
#include "Sensor.h"
#include "SensorTime.h"


/*ChibiOS*/
#include <ChRt.h>
MUTEX_DECL(QueueMod_Mtx);

#define READ_INTERVAL 1

//The queues hold pointers to the starting addresses of each block of size BLOCK_SIZE.
//This is done to optimize performance and simplify later code.
//These pointers, once initialized, will basically go back and forth between
//the emptyQueue and the writeQueue
Queue<uint8_t*> writeQueue = Queue<uint8_t*>(BLOCK_COUNT);
Queue<uint8_t*> emptyQueue = Queue<uint8_t*>(BLOCK_COUNT);

uint8_t blockBuf[BUFFER_SIZE];

SdExFat sd;
ExFatFile binFile;


const uint16_t numSensors = 1;
Sensor* sensors[20];
uint8_t packetBuf[MAX_PACKET_SIZE];

const uint32_t PREALLOCATE_SIZE_MiB = 2UL;
const uint64_t PREALLOCATE_SIZE  =  (uint64_t)PREALLOCATE_SIZE_MiB << 20;



void queueBuf(uint8_t* buf, uint16_t size) {
    static uint8_t* currentBlock = emptyQueue.pop(); //On init, get an empty block.
    static uint32_t currentInd = 0;

    for(int i = 0; i < size; i++) {
        //Add a val to the current block...
        currentBlock[currentInd] = buf[i];
        currentInd++;

        //If we have filled the block, push it to the write queue and get an empty one.
        if(currentInd >= BLOCK_SIZE) {
            //Serial.println("Block filled, getting new.");

            //If there's an empty block we can write to, pop it make it ours owo.
            if(emptyQueue.count() > 0) {
                writeQueue.push(currentBlock);
                currentBlock = emptyQueue.pop();
            } else {
                Serial.println("BUFFER OVERRAN!");
                SysCall::halt();
            }

            //Reset the block index.
            currentInd = 0;
        }

    }
}


THD_WORKING_AREA(readerWa, 512);
THD_FUNCTION(reader, arg) {
    static systime_t nextRead = chVTGetSystemTime();
    while(true) {
        uint16_t size = BufferPacket(packetBuf, sensors, numSensors);

        chMtxLock(&QueueMod_Mtx);
        queueBuf(packetBuf, size);
        chMtxUnlock(&QueueMod_Mtx);
        
        nextRead += TIME_MS2I(1);
        chThdSleepUntil(nextRead);
    }
    
}


void chMain() {
    static uint32_t falls = 0;
    chThdCreateStatic(  readerWa,
                        sizeof(readerWa),
                        NORMALPRIO + 10,
                        reader,
                        NULL);




    while(true) {

        if(!sd.card()->isBusy() && writeQueue.count() > 0) { //If card not busy and data availible, write.
            Serial.println("Writing...");

            //Pop a block pointer from the write queue, and write it to SD.
            chMtxLock(&QueueMod_Mtx);
            uint8_t* poppedBlock = writeQueue.pop();
            chMtxUnlock(&QueueMod_Mtx);

            size_t written = binFile.write(poppedBlock, BLOCK_SIZE);
            if(written != BLOCK_SIZE) Serial.println("Write Failed! :(");
            
            //Once done, put the written back on the empty queue to be used again.
            chMtxLock(&QueueMod_Mtx);
            emptyQueue.push(poppedBlock);
            chMtxUnlock(&QueueMod_Mtx);
        }



        if(Serial.available() > 0) {
            binFile.truncate();
            binFile.sync();
            Serial.println("Finished.");
            Serial.print("Skipped ");
            Serial.print(falls);
            Serial.print("ms over ");
            Serial.print(millis());
            Serial.print("ms for a ");
            Serial.print(((float) falls / (float) millis()) * 100);
            Serial.println("% failiure rate.");
            return; //Kill the thread.
        }
    }
}

void setup() {
    Serial.begin(115200);
    while(!Serial){;}
    Serial.println("Starting...");

    if(!sd.begin(254)) {
        sd.initErrorHalt(&Serial);
    }
    char fileName[FILENAME_SIZE];
    SelectNextFilename( fileName , &sd);

    if(!binFile.open(fileName, O_RDWR | O_CREAT)) {
        Serial.println("File Open Err.");
    }

    if(!binFile.preAllocate(PREALLOCATE_SIZE)) {
        Serial.println("Preallocation Err.");
    }

    
    Serial.print(F("preAllocated: "));
    Serial.print(PREALLOCATE_SIZE_MiB);
    Serial.println(F(" MiB"));


    /*Sensors*/
    sensors[0] = new SensorTime();


    //Populate the empty queue with pointers to each 512 byte-long buffer.
    for(uint32_t i = 0; i < BLOCK_COUNT; i++) {
        uint8_t* bufIndex = &blockBuf[i * BLOCK_SIZE]; //Pointer generated to be at the start of each block.
        emptyQueue.push(bufIndex);
    }

    chBegin(chMain);
}



void loop() {

}
