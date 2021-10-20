#include <SdFat.h>
//https://github.com/sdesalas/Arduino-Queue.h
#include <queue.h>
#include "config.h"
#include "util.h"
#include "macros.h"
#include "LED.h"


/*Sensors*/
#include "Sensor.h"
#include "SensorTime.h"
#include "SensorMarker.h"
#include "SensorBrakePressure.h"
#include "SensorRotSpeeds.h"
#include "SensorMPU6050.h"

#include "DebouncedButton.h"


/*ChibiOS*/
#include <ChRt.h>
MUTEX_DECL(QueueMod_Mtx);

//The queues hold pointers to the starting addresses of each block of size BLOCK_SIZE in blockBuf.
//This is done to optimize performance and simplify later code.
//These pointers, once initialized, will basically be passed back and forth between
//the emptyQueue and the writeQueue
Queue<uint8_t*> writeQueue = Queue<uint8_t*>(BLOCK_COUNT);
Queue<uint8_t*> emptyQueue = Queue<uint8_t*>(BLOCK_COUNT);

//Holds the actual memeory of the blocks accessed through pointers in the write/empty queue
uint8_t blockBuf[BUFFER_SIZE];

Sensor* sensors[SENSOR_ARR_SIZE];
uint16_t numSensors = 0; //BC sensors are semi-dynamically created (through config),


uint8_t packetBuf[MAX_PACKET_SIZE];

LED statusLed(PIN_STATUS_LED);

volatile bool isLogging = false;


/*Func defs*/
void fsmWriter();

/*Takes two paremeters - a pointer to a buffer and a size - and
writes the passed buffer into blocks within the block buffer, moving blocks from 
the empty buffer to the full buffer as they fill.*/
void queueBuf(uint8_t* buf, uint16_t size) {
    static uint8_t* currentBlock = emptyQueue.pop(); //On init, get an empty block.
    static uint32_t currentInd = 0;

    for(int i = 0; i < size; i++) {
        //Add a val to the current block...
        currentBlock[currentInd] = buf[i];
        currentInd++;

        //If we have filled the block, push it to the write queue and get an empty one.
        if(currentInd >= BLOCK_SIZE) {

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




/*Reader Thread - every SAMPLE_INTERVAL ms this function will run, recording sensor data into the block buffer.*/
THD_WORKING_AREA(readerWa, 512);
THD_FUNCTION(reader, arg) {
    static systime_t nextRead = chVTGetSystemTime();
    while(true) {
        uint16_t size = BufferPacket(packetBuf, sensors, numSensors);
        sensorPrint("\n");
        if(isLogging) {
            chMtxLock(&QueueMod_Mtx);
            queueBuf(packetBuf, size);
            chMtxUnlock(&QueueMod_Mtx);
        }


        nextRead += TIME_MS2I(SAMPLE_INTERVAL);
        chThdSleepUntil(nextRead);
    }
    
}


[[noreturn]] void chMain() {
    Serial.println("Starting!");
    chThdCreateStatic(readerWa, sizeof(readerWa), READER_PRIORITY, reader, NULL);

    while(true) {
        for(int i = 0; i < numSensors; i++) {
            sensors[i]->loop();
        }
        fsmWriter();
        statusLed.tick();
    }
}



void setup() {
    Serial.begin(115200);

    uint32_t timeoutAt = millis() + SERIAL_TIMEOUT;
    statusLed.setState(LED::FAST_BLINK);
    while(!Serial && timeoutAt > millis()){statusLed.tick();}
    debugl("Starting...");

    /*Sensor setups*/
    #ifdef SENSOR_TIME
        sensors[numSensors++] = new SensorTime();
    #endif
    
    #ifdef SENSOR_ECVT 
        Serial.println("ECVT not yet implmented..");
    #endif
    
    #ifdef SENSOR_MARKER 
        sensors[numSensors++] = new SensorMarker(PIN_MARKER_BTN);
    #endif
    
    #ifdef SENSOR_BRAKEPRESSURE 
        sensors[numSensors++] = new SensorBrakePressures(PIN_BPRESSURE_F, PIN_BPRESSURE_R);
    #endif
    
    #ifdef SENSOR_ROTATIONSPEEDS 
    {
        static uint16_t index = numSensors++; //Get an index to reference the rotationspeed sensor.
        sensors[index] = new SensorRotSpeeds();

        auto engineISR = [](){ ((SensorRotSpeeds*) sensors[index])-> calcESpeed(); };
        auto rWheelISR = [](){ ((SensorRotSpeeds*) sensors[index])->calcRWheels(); };
        auto lFrontISR = [](){ ((SensorRotSpeeds*) sensors[index])->calcFLWheel(); };
        auto rFrontISR = [](){ ((SensorRotSpeeds*) sensors[index])->calcFRWheel(); };

        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_ENG), engineISR, RISING);
        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_RGO), rWheelISR, RISING);
        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_WFL), lFrontISR, RISING);
        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_WFR), rFrontISR, RISING);
    }
    #endif

    #ifdef SENSOR_MPU6050
    {
        static uint16_t index = numSensors++; //Get an index to reference the rotationspeed sensor.
        sensors[index] = new SensorMPU6050();
        auto mpuISR = [](){ ((SensorMPU6050*) sensors[index])->dataReady(); };
        attachInterrupt(digitalPinToInterrupt(A17), mpuISR, RISING);
    }
    #endif

    Serial.print(numSensors);
    Serial.println(" sensors initialized!");

    //Populate the empty queue with pointers to each 512 byte-long buffer.
    for(uint32_t i = 0; i < BLOCK_COUNT; i++) {
        uint8_t* bufIndex = &blockBuf[i * BLOCK_SIZE]; //Pointer generated to be at the start of each block.
        emptyQueue.push(bufIndex);
    }

    statusLed.setState(LED::CONSTANT_BLINK);


    chBegin(chMain);
}



void loop() {

}



void fsmWriter() {
    //Positive states are OK states
    static const int STATE_WAITING_TO_START = 0;
    static const int INITIALIZING_SD = 1;
    static const int STATE_STARTING = 2; 
    static const int STATE_WAITING_TO_STOP = 3; 
    static const int STATE_WRITING = 4; 
    static const int STATE_STOPPING = 5; 

    //Negitive states are error(ish) states
    static const int STATE_WAITING_FOR_SD_INIT = -1;
    static const int STATE_SIGNAL_SD_ERR = -2;

    //Handles debouncing of the logger toggle button.
    static DebouncedButton loggerBtn(PIN_LOGGER_BTN, false);

    STATE_DEFINITIONS; //Defines variables needed for state machine function.

    static SD_TYPE sd;
    static FILE_TYPE file;

    static LED writerLed(PIN_LOGGER_LED);

    switch (state){
        case INITIALIZING_SD: {
            debugl("Trying start...");
            if(sd.begin(254)) {
                SET_STATE(STATE_STARTING);
                debugl("starting...");
            } else {
                Serial.println("SD INTIALIZATION ERROR");
                sd.printSdError(&Serial);
                SET_STATE(STATE_SIGNAL_SD_ERR);
            }
            break;
        }

        case STATE_SIGNAL_SD_ERR: {
            static uint32_t timeout = 0;
            ON_STATE_ENTER({
                writerLed.setState(LED::FAST_BLINK);
                timeout = millis() + 1000;
            });

            if(millis() > timeout) {
                SET_STATE(STATE_WAITING_FOR_SD_INIT);
            }
        }

        case STATE_WAITING_FOR_SD_INIT: {
            ON_STATE_ENTER({
                writerLed.setState(LED::OFF);
                isLogging = false;
            });

            if(loggerBtn.isTriggered()) {
                SET_STATE(INITIALIZING_SD);
            }
        }
        break;




        case STATE_WAITING_TO_START: //WAITING TO START
            SET_STATE_IF(loggerBtn.isTriggered(), INITIALIZING_SD);
            break;



        case STATE_STARTING: {
            char fileName[FILENAME_SIZE];
            
            SelectNextFilename( fileName, &sd);
            
            //Open the selected fileName. If there's an error, signal it.
            SET_STATE_IF(!file.open(fileName, O_RDWR | O_CREAT), STATE_SIGNAL_SD_ERR);

            //Initialize all sensors for this run.
            for(uint8_t i = 0; i < numSensors; i++) {
                sensors[i]->start();
            }

            //Place all the previously populated blocks into the empty queue
            for(uint16_t i = 0; i < writeQueue.count(); i++) {
                emptyQueue.push(writeQueue.pop());
            }

            writerLed.setState(LED::ON);
            isLogging = true;

            SET_STATE(STATE_WAITING_TO_STOP);
            break;
        } 



        case STATE_WAITING_TO_STOP: {
            SET_STATE_IF(!sd.card()->isBusy() && writeQueue.count() > 0, STATE_WRITING);  //If card not busy and data availible, write.
            //debugl(digitalRead(PIN_LOGGER_BTN));
            SET_STATE_IF(loggerBtn.isTriggered(), STATE_STOPPING);
            break;
        }

        case STATE_WRITING: {
            //Pop a block pointer from the write queue, and write it to SD. 
            //Prevent other threads from touching the queue while this happens through the mtx.
            chMtxLock(&QueueMod_Mtx);
            uint8_t* poppedBlock = writeQueue.pop();
            chMtxUnlock(&QueueMod_Mtx);

            size_t written = file.write(poppedBlock, BLOCK_SIZE); //Can do this w/o checking block B/C if we're here, data is guarenteed to be in queue.
            if(written != BLOCK_SIZE) {
                debugl("Write Failed! :(");
                SET_STATE(STATE_WAITING_FOR_SD_INIT);
                }
            
            //Once done, put the written back on the empty queue to be used again.
            chMtxLock(&QueueMod_Mtx);
            emptyQueue.push(poppedBlock);
            chMtxUnlock(&QueueMod_Mtx);

            
            state = STATE_WAITING_TO_STOP;
            break;
        }

        case STATE_STOPPING: {
            isLogging = false;
            file.truncate();
            file.sync();
            file.close();

            //Denitialize all sensors for this run.
            for(uint8_t i = 0; i < numSensors; i++) {
                sensors[i]->stop();
            }
            Serial.println("Stopping logging!");
            writerLed.setState(LED::OFF);

            SET_STATE(STATE_WAITING_TO_START);
        }
    }

    writerLed.tick();
}