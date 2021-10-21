#include <SdFat.h>
//https://github.com/sdesalas/Arduino-Queue.h
#include <queue.h>
#include "config.h"
#include "util.h"
#include "macros.h"
#include "leds.h"
#include "fsm.h"
#include "SensorManager.h"
#include "RF24.h"

/*sensors*/
#include "Sensor.h"
#include "sensors/SensorTime.h"
#include "sensors/SensorMarker.h"
#include "sensors/SensorBrakePressure.h"
#include "sensors/SensorRotSpeeds.h"
#include "sensors/SensorMPU6050.h"

#include "DebouncedButton.h"


/*ChibiOS*/
#include <ChRt.h>

MUTEX_DECL(QueueMod_Mtx);

//The queues hold pointers to the starting addresses of each block of size BLOCK_SIZE in blockBuf.
//This is done to optimize performance and simplify later code.
//These pointers, once initialized, will basically be passed back and forth between
//the emptyQueue and the writeQueue
Queue<uint8_t *> writeQueue = Queue<uint8_t *>(BLOCK_COUNT);
Queue<uint8_t *> emptyQueue = Queue<uint8_t *>(BLOCK_COUNT);

//Holds the actual memeory of the blocks accessed through pointers in the write/empty queue
uint8_t blockBuf[BUFFER_SIZE];

Sensor *sensors[SENSOR_ARR_SIZE];
uint16_t numSensors = 0; //BC sensors are semi-dynamically created (through config),


uint8_t packetBuf[MAX_PACKET_SIZE];


volatile bool isLogging = false;

SensorManager<10, 512> manager;

/*Func defs*/
void fsmWriter();

/*Takes two paremeters - a pointer to a buffer and a size - and
writes the passed buffer into blocks within the block buffer, moving blocks from 
the empty buffer to the full buffer as they fill.*/
void queueBuf(uint8_t *buf, uint16_t size) {
    static uint8_t *currentBlock = emptyQueue.pop(); //On init, get an empty block.
    static uint32_t currentInd = 0;

    for (int i = 0; i < size; i++) {
        //Add a val to the current block...
        currentBlock[currentInd] = buf[i];
        currentInd++;

        //If we have filled the block, push it to the write queue and get an empty one.
        if (currentInd >= BLOCK_SIZE) {

            //If there's an empty block we can write to, pop it make it ours owo.
            if (emptyQueue.count() > 0) {
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

[[noreturn]] void thd_sensor_reader(void *arg) {
    static systime_t nextRead = chVTGetSystemTime();
    while (true) {
        uint16_t size = BufferPacket(packetBuf, sensors, numSensors);
        sensorPrint("\n");

        if (isLogging) {
            chMtxLock(&QueueMod_Mtx);
            queueBuf(packetBuf, size);
            chMtxUnlock(&QueueMod_Mtx);
        }


        nextRead += TIME_MS2I(SAMPLE_INTERVAL);
        chThdSleepUntil(nextRead);
    }
}


[[noreturn]] void chMain() {
    debugl("Starting main thread...");

    chThdCreateStatic(readerWa, sizeof(readerWa), READER_PRIORITY, thd_sensor_reader, nullptr);

    while (true) {
        for (int i = 0; i < numSensors; i++)
            sensors[i]->loop();

        fsmWriter();
        status_led.Update();
    }
}


void setup() {
    Serial.begin(115200);

    uint32_t timeoutAt = millis() + SERIAL_TIMEOUT;
    status_led.Blink(100, 100);
    while (!Serial && timeoutAt > millis()) { status_led.Update(); }
    debugl("Starting...");

    /*Sensor setups*/
#ifdef SENSOR_TIME
    sensors[numSensors++] = new SensorTime(0x01);
#endif

#ifdef SENSOR_ECVT
    Serial.println("ECVT not yet implmented..");
#endif

#ifdef SENSOR_MARKER
    sensors[numSensors++] = new SensorMarker(0x03, PIN_MARKER_BTN);
#endif

#ifdef SENSOR_BRAKEPRESSURE
    sensors[numSensors++] = new SensorBrakePressures(0x04, PIN_BPRESSURE_F, PIN_BPRESSURE_R);
#endif

#ifdef SENSOR_ROTATIONSPEEDS
    {
        static uint16_t index = numSensors++; //Get an index to reference the rotationspeed sensor.
        sensors[index] = new SensorRotSpeeds(0x05);

        auto engineISR = []() { ((SensorRotSpeeds *) sensors[index])->calcESpeed(); };
        auto rWheelISR = []() { ((SensorRotSpeeds *) sensors[index])->calcRWheels(); };
        auto lFrontISR = []() { ((SensorRotSpeeds *) sensors[index])->calcFLWheel(); };
        auto rFrontISR = []() { ((SensorRotSpeeds *) sensors[index])->calcFRWheel(); };

        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_ENG), engineISR, RISING);
        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_RGO), rWheelISR, RISING);
        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_WFL), lFrontISR, RISING);
        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_WFR), rFrontISR, RISING);
    }
#endif

#ifdef SENSOR_MPU6050
    {
        static uint16_t index = numSensors++; //Get an index to reference the rotationspeed sensor.
        sensors[index] = new SensorMPU6050(0x06);
        auto mpuISR = []() { ((SensorMPU6050 *) sensors[index])->dataReady(); };
        attachInterrupt(digitalPinToInterrupt(PIN_MPU6050_IRQ), mpuISR, RISING);
    }
#endif

    debug(numSensors);
    debugl(" sensors initialized!");

    //Populate the empty queue with pointers to each 512 byte-long buffer.
    for (uint32_t i = 0; i < BLOCK_COUNT; i++) {
        uint8_t *bufIndex = &blockBuf[i * BLOCK_SIZE]; //Pointer generated to be at the start of each block.
        emptyQueue.push(bufIndex);
    }
    status_led.Blink(500, 500);

    chBegin(chMain);
}


void fsmWriter() {
    //Positive states are OK states
    enum state_e {
        STATE_WAITING_TO_START,
        STATE_LOGGING,
        STATE_WRITING_SD,
        STATE_STOPPING,
        STATE_SIGNAL_SD_ERR
    };

    //Handles debouncing of the logger toggle button.
    static DebouncedButton loggerBtn(PIN_LOGGER_BTN, false);

    static state_e state = STATE_WAITING_TO_START;
    static state_e lastState = STATE_WAITING_TO_START;

    static SD_TYPE sd;
    static RF24 radio(PIN_NRF_CE, PIN_NRF_CS); // using pin 7 for the CE pin, and pin 8 for the CSN pin

    static FILE_TYPE file;


    switch (state) {
        case STATE_SIGNAL_SD_ERR: {
            static uint32_t timeout = 0;
            ON_STATE_ENTER({
                               Serial.println("SD INITIALIZATION ERROR");
                               sd.printSdError(&Serial);
                               logger_led.Blink(100, 100);
                               timeout = millis() + 1000;
                           });
            SET_STATE_IF(millis() > timeout, STATE_WAITING_TO_START);
        }

        case STATE_WAITING_TO_START: {
            ON_STATE_ENTER({
                               logger_led.Off();
                               isLogging = false;
                           });
            SET_STATE_IF(loggerBtn.isTriggered(), STATE_LOGGING);
            break;
        }


        case STATE_LOGGING: {
            ON_STATE_ENTER(
                    {
                        debugl("Trying to initialize SD...");
                        SET_STATE_IF(!sd.begin(PIN_SD_CS), STATE_SIGNAL_SD_ERR)

                        char fileName[FILENAME_SIZE];
                        SelectNextFilename(fileName, &sd);

                        //Open the selected fileName. If there's an error, signal it.
                        SET_STATE_IF(!file.open(fileName, O_RDWR | O_CREAT), STATE_SIGNAL_SD_ERR);

                        //Initialize all sensors for this run.
                        for (int i = 0; i < numSensors; i++)
                            sensors[i]->start();

                        //Place all the previously populated blocks into the empty queue
                        for (int i = 0; i < writeQueue.count(); i++)
                            emptyQueue.push(writeQueue.pop());

                        logger_led.On();
                        isLogging = true;
                    }
            );

            //If card not busy and data available, write.
            SET_STATE_IF(!sd.card()->isBusy() && writeQueue.count() > 0, STATE_WRITING_SD);
            SET_STATE_IF(loggerBtn.isTriggered(), STATE_STOPPING);
            break;
        }

        case STATE_WRITING_SD: {
            //Pop a block pointer from the write queue, and write it to SD. 
            //Prevent other threads from touching the queue while this happens through the mtx.
            chMtxLock(&QueueMod_Mtx);
            uint8_t *poppedBlock = writeQueue.pop();
            chMtxUnlock(&QueueMod_Mtx);

            size_t written = file.write(poppedBlock,
                                        BLOCK_SIZE); //Can do this w/o checking block B/C if we're here, data is guarenteed to be in queue.
            if (written != BLOCK_SIZE) {
                debugl("Write Failed! :(");

                for (int i = 0; i < numSensors; i++)
                    sensors[i]->stop();

                SET_STATE(STATE_SIGNAL_SD_ERR);
            }

            //Once done, put the written back on the empty queue to be used again.
            chMtxLock(&QueueMod_Mtx);
            emptyQueue.push(poppedBlock);
            chMtxUnlock(&QueueMod_Mtx);

            SET_STATE(STATE_LOGGING)
            break;
        }

        case STATE_STOPPING: {
            isLogging = false;
            file.truncate();
            file.sync();
            file.close();

            //Denitialize all sensors for this run.
            for (size_t i = 0; i < numSensors; i++)
                sensors[i]->stop();

            Serial.println("Stopping logging!");
            logger_led.Off();

            SET_STATE(STATE_WAITING_TO_START);
        }
    }

    logger_led.Update();
}

//Just here so arduino can compile the program.
void loop() {}