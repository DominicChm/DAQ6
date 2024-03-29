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
#include <ChRt.h>

/*sensors*/
#include "Sensor.h"
#include "sensors/SensorTime.h"
#include "sensors/SensorMarker.h"
#include "sensors/SensorBrakePressure.h"
#include "sensors/SensorRotSpeeds.h"
#include "sensors/SensorMPU6050.h"

#include "DebouncedButton.h"
#include "DataBlocker.h"


DataBlocker<uint8_t, SD_BLOCK_SIZE, SD_BLOCK_COUNT> sd_blocker;
DataBlocker<uint8_t, NRF_BLOCK_SIZE, NRF_BLOCK_COUNT> nrf_blocker;

Sensor *sensors[SENSOR_ARR_SIZE];
uint16_t numSensors = 0; //BC sensors are semi-dynamically created (through config),


uint8_t packetBuf[MAX_PACKET_SIZE];

volatile bool isLogging = false;
volatile bool radio_enabled = false;
volatile bool radio_transmitting = false;
volatile bool interactive_serial = true;
//SensorManager<10, 512> manager;
#define INT_PRINTLN(arg) if(interactive_serial) Serial.println(arg)
#define INT_PRINT(arg) if(interactive_serial) Serial.print(arg)


RF24 radio(PIN_NRF_CE, PIN_NRF_CS);

/*Func defs*/
void sd_writer_fsm();

void nrf_writer();

void IRQ_nrf();

void serial_driver();


/*Reader Thread - every SAMPLE_INTERVAL ms this function will run, recording sensor data into the block buffer.*/
THD_WORKING_AREA(readerWa, 512);

[[noreturn]] void thd_sensor_reader(void *arg) {
    static systime_t nextRead = chVTGetSystemTime();
    while (true) {
        uint16_t size = BufferPacket(packetBuf, sensors, numSensors);
        sensorPrint("\n");

        if (isLogging)
            sd_blocker.write(packetBuf, size);

        if (radio_enabled)
            nrf_blocker.write(packetBuf, size);

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

        nrf_writer();
        sd_writer_fsm();
        serial_driver();
        status_led.Update();
    }
}


void setup() {
    Serial.begin(115200);

    //Wait for serial connection
    uint32_t timeoutAt = millis() + SERIAL_TIMEOUT;
    status_led.Blink(100, 100).Forever();
    while (!Serial && timeoutAt > millis()) { status_led.Update(); }

    debugl("Starting...");

    //Set SPI to use alternate pin setup. Done B/C of way PCB was wired. Not necessary if default SPI
    //pins are routed.
    SPI.setMOSI(7);
    SPI.setMISO(8);
    SPI.setSCK(14);

    radio_enabled = radio.begin();
    if (radio_enabled) {
        debugl("NRF24l01+ Connected.");
        radio.setPayloadSize(NRF_BLOCK_SIZE); // char[7] & uint8_t datatypes occupy 8 bytes
        radio.setPALevel(NRF_PA_LEVEL); // RF24_PA_MAX is default.
        radio.openWritingPipe((uint8_t *) NRF_CAR_ADDRESS);
        radio.stopListening();
        radio.setAutoAck(false);
        radio.setDataRate(RF24_1MBPS);
        radio.setCRCLength(RF24_CRC_8);
        radio.maskIRQ(false, true, true); //Only care about TX ok events.
        radio.csDelay = 0;
        radio.txDelay = 0;
        radio.startListening();
        radio.stopListening();

        attachInterrupt(digitalPinToInterrupt(PIN_NRF_IRQ), IRQ_nrf, FALLING);
    } else debugl("Failed to init NRF24l01+ radio module. Continuing without.");

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

        auto engineISR = []() { ((SensorRotSpeeds *) sensors[index])->ePulseISR(); };
        auto rWheelISR = []() { ((SensorRotSpeeds *) sensors[index])->rWheelPulseISR(); };
//        auto lFrontISR = []() { ((SensorRotSpeeds *) sensors[index])->calcFLWheel(); };
//        auto rFrontISR = []() { ((SensorRotSpeeds *) sensors[index])->calcFRWheel(); };

        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_ENG), engineISR, RISING);
        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_RGO), rWheelISR, RISING);
//        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_WFL), lFrontISR, RISING);
//        attachInterrupt(digitalPinToInterrupt(PIN_RSPEED_WFR), rFrontISR, RISING);
    }
#endif

#ifdef SENSOR_MPU6050
    {
        static uint16_t index = numSensors++; //Get an index to reference the rotationspeed sensor.
        sensors[index] = new SensorMPU6050(0x06);
        auto mpuISR = []() { ((SensorMPU6050 *) sensors[index])->dataReady(); };
        attachInterrupt(digitalPinToInterrupt(PIN_MPU6050_IRQ), mpuISR, FALLING);
    }
#endif

    debug(numSensors);
    debugl(" sensors initialized!");

    status_led.Blink(500, 500).Forever();

    chBegin(chMain);
}

void IRQ_nrf() {
    radio_transmitting = false;
    radio.txStandBy();
    radio.startListening();                 // Put the radio into listening (RX) mode
}


namespace serialcmd {
    SD_TYPE sd;

    bool init_sd() {
        if (isLogging) {
            Serial.println("CAN'T LIST - LOGGING IN PROGRESS");
            return false;
        }

        if (!sd.begin(PIN_SD_CS)) {
            Serial.println("FAILED TO OPEN SD");
            return false;
        }

        return true;
    }

    void printDirectory(ExFile dir) {
        ExFile entry = dir.openNextFile(); //Get rid of System Volume Information file
        while (true) {
            entry = dir.openNextFile();
            if (!entry)
                break;

            char name[127]{};
            entry.getName(name, 127);
            if (interactive_serial) {
                Serial.print(name);

                if (!entry.isDirectory()) {
                    Serial.print("    ");
                    Serial.print(entry.size(), DEC);
                    Serial.print(" bytes");
                }
                Serial.println();
            } else {
                Serial.println(name);
            }
            entry.close();
        }
    }

    void list(char *command) {
        INT_PRINTLN("LIST COMMAND");
        if (!init_sd()) return;


        ExFile root = sd.open("/");
        serialcmd::printDirectory(root);
    }

    void download_bin(char *buf) {
        if (!init_sd()) return;
        char *dirArg = strtok(nullptr, " "); //Use strtok setup from calling fn.

        if (dirArg == nullptr) {
            INT_PRINTLN("No directory argument passed!");
            return;
        } else if (!sd.exists(dirArg)) {
            INT_PRINT("Couldn't find file ");
            INT_PRINTLN(dirArg);

            return;
        }

        INT_PRINTLN("START");

        ExFile f = sd.open(dirArg);

        while (f.available())
            Serial.write(f.read());

        INT_PRINTLN("DONE");



        //if(!sd.exists())

    }

    void download_csv(char *buf) {
        INT_PRINTLN("DL_CSV");
        INT_PRINTLN("DL-CSV UNIMPLEMENTED");
        if (!init_sd()) return;
    }

    void help(char *buf) {
        INT_PRINTLN("HELP:");
        INT_PRINTLN("list - Lists all files on SD");
        INT_PRINTLN("dl <FILE> - downloads file in CSV (UNIMPLEMENTED)");
        INT_PRINTLN("dl-bin <FILE> - downloads file in BIN");
        INT_PRINTLN("clear-sd - CAUTION: Deletes all stored files");

    }

    void set_interactive(char *buf) {
        char *keyword = strtok(nullptr, " ");
        interactive_serial = !!strcmp(keyword, "off");
    }

    void clear_sd(char *buf) {
        if (!init_sd()) return;
        INT_PRINTLN("DELETING ALL FILES...!");

        ExFile root = sd.open("/");
        ExFile f = root.openNextFile();
        while (f) {
            f.remove();
        }
        INT_PRINTLN("DONE!");

    }
}

void parse_serial_command(char *buf) {
/* get the first token */
    char *token = strtok(buf, " ");

    INT_PRINTLN(token);

    if (!strcmp(token, "list"))
        serialcmd::list(buf);

    else if (!strcmp(token, "dl-bin"))
        serialcmd::download_bin(buf);

    else if (!strcmp(token, "dl"))
        serialcmd::download_csv(buf);

    else if (!strcmp(token, "help"))
        serialcmd::help(buf);

    else if (!strcmp(token, "int"))
        serialcmd::set_interactive(buf);

    else if (!strcmp(token, "clear-sd"))
        serialcmd::clear_sd(buf);

    else INT_PRINTLN("Unrecognized command");
}

void serial_driver() {
    static char buf[256];
    static int head = 0;

    if (!Serial.available()) return;
    char c = Serial.read();

    switch (c) {
        case '\r':
        case '\n':
            if (head <= 0) break; //Don't parse empty buffers.

            buf[head++] = '\0';
            parse_serial_command(buf);

            head = 0;
            break;
        default:
            buf[head++] = c;
    }


}


void nrf_writer() {
    if (nrf_blocker.available() && !radio_transmitting) {
        uint8_t *block = nrf_blocker.checkout_block();
        radio.stopListening();
        radio.startFastWrite(block, nrf_blocker.size(), true);
        radio_transmitting = true;

        nrf_blocker.return_block(block);
    }
}

void sd_writer_fsm() {
    //Positive states are OK states
    enum state_e {
        STATE_WAITING_TO_START,
        STATE_LOGGING,
        STATE_WRITING_SD,
        STATE_STOPPING,
        STATE_SIGNAL_SD_ERR,
        STATE_INIT_LOGGING,
    };

    //Handles debouncing of the logger toggle button.
    static DebouncedButton loggerBtn(PIN_LOGGER_BTN, false);

    static state_e state = STATE_WAITING_TO_START;
    static state_e lastState = STATE_WAITING_TO_START;

    static SD_TYPE sd;
    //static RF24 radio(PIN_NRF_CE, PIN_NRF_CS); // using pin 7 for the CE pin, and pin 8 for the CSN pin

    static FILE_TYPE file;


    switch (state) {
        case STATE_SIGNAL_SD_ERR: {
            static uint32_t timeout = 0;
            ON_STATE_ENTER({
                               Serial.println("SD INITIALIZATION ERROR");
                               sd.printSdError(&Serial);
                               logger_led.Blink(100, 100).Forever();
                               timeout = millis() + 1000;
                           });
            SET_STATE_IF(millis() > timeout, STATE_WAITING_TO_START);
        }

        case STATE_WAITING_TO_START: {
            ON_STATE_ENTER({
                               logger_led.Off();
                               isLogging = false;
                           });
            SET_STATE_IF(loggerBtn.isTriggered(), STATE_INIT_LOGGING);
            break;
        }
        case STATE_INIT_LOGGING: {
            debugl("Trying to initialize SD...");
            SET_STATE_IF(!sd.begin(PIN_SD_CS), STATE_SIGNAL_SD_ERR)

            char fileName[FILENAME_SIZE];
            SelectNextFilename(fileName, &sd);

            //Open the selected fileName. If there's an error, signal it.
            SET_STATE_IF(!file.open(fileName, O_RDWR | O_CREAT), STATE_SIGNAL_SD_ERR);

            //Initialize all sensors for this run.
            for (int i = 0; i < numSensors; i++)
                sensors[i]->start();

            logger_led.On();
            isLogging = true;
            SET_STATE(STATE_LOGGING);
            break;
        }
        case STATE_LOGGING: {
            //If card not busy and data available, write.
            SET_STATE_IF(!sd.card()->isBusy() && sd_blocker.available(), STATE_WRITING_SD);
            SET_STATE_IF(loggerBtn.isTriggered(), STATE_STOPPING);
            break;
        }

        case STATE_WRITING_SD: {
            //Pop a block pointer from the write queue, and write it to SD. 
            //Prevent other threads from touching the queue while this happens through the mtx.
            uint8_t *popped_block = sd_blocker.checkout_block();

            //Can do this w/o checking block B/C if we're here, data is guarenteed to be in queue.
            int written = (int) file.write(popped_block, sd_blocker.size());

            if (written != sd_blocker.size()) {
                debugl("Write Failed! :(");

                for (int i = 0; i < numSensors; i++)
                    sensors[i]->stop();

                SET_STATE(STATE_SIGNAL_SD_ERR);
            }

            SET_STATE(STATE_LOGGING)
            sd_blocker.return_block(popped_block);
            break;
        }

        case STATE_STOPPING: {
            isLogging = false;

            //Write remaining data to file.
            uint8_t *block = sd_blocker.checkout_remainder_block();
            file.write(block, sd_blocker.remainder_size());
            sd_blocker.return_block(block);

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