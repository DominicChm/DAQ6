/*DEBUG CONFIG*/
#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG

//#define PRINT_SENSORS

/*BUFFER QUEUE CONFIG*/
#define SD_BLOCK_SIZE 512
#define SD_BLOCK_COUNT 10

#define NRF_BLOCK_SIZE 32
#define NRF_BLOCK_COUNT 30

#define MAX_PACKET_SIZE 1024

/*FILE CONFIG*/
//All credit for the filename selection code goes to Rahul
#define FILENAME_PREFIX "D"
#define FILENAME_SUFFIX ".bin"



/* Built in ChibiOS priorities:

zero	    Reserved priority level, all the possible priority levels are guaranteed to be greater than zero.
IDLEPRIO	Special priority level reserved for the Idle Thread.
LOWPRIO	    Lowest priority level usable by user threads.
NORMALPRIO	Central priority level, the main() thread is started at this level. User thread priorities are usually allocated around this central priority.
HIGHPRIO	Highest priority level usable by user threads. Above this level the priorities are reserved.
ABSPRIO	    Absolute priority level, highest reserved priority level. Above this levels there are the hardware priority levels used by interrupt sources.
*/

/*THREADING CONFIG*/
#define READER_PRIORITY NORMALPRIO + 10

/***PIN CONFIG***/
//NRF24l01+ Pin Config
#define PIN_NRF_CE 35
#define PIN_NRF_CS 15
#define PIN_NRF_IRQ 12

//SD Card config - 254 is Teensy default CS pin.
#define PIN_SD_CS 254

//Pin config for logger control - toggling and status.
#define PIN_LOGGER_LED 4
#define PIN_LOGGER_BTN 2

//Pin for auxillary status LED - 13 is internal Teensy LED
#define PIN_STATUS_LED 13

//Marker button pin.
#define PIN_MARKER_BTN 3
#define PIN_MARKER_LED 17 //<FILL THIS IN LATER!!!!!!>

//Pins for the wheelspeed sensors. These are just hall effect sensors.
//ENG = engine, RGO = Rear Gearbox Output, WFL = Wheel Front Left, WFR = Wheel Front Right
#define PIN_RSPEED_ENG  5
#define PIN_RSPEED_RGO  6
#define PIN_RSPEED_WFL 29
#define PIN_RSPEED_WFR 30

//Brake Pressure transducer pins
#define PIN_BPRESSURE_F 34
#define PIN_BPRESSURE_R 33

//MPU6050
#define PIN_MPU6050_IRQ 36

/*SENSOR CONFIG*/

//Sample interval in ms.
#define SAMPLE_INTERVAL 1000/30

//Comment a define to disable a sensor.
#define SENSOR_TIME

//#define SENSOR_ECVT

#define SENSOR_MARKER

#define SENSOR_BRAKEPRESSURE

#define SENSOR_ROTATIONSPEEDS

#define SENSOR_MPU6050

/*CONTROLS CONFIG*/
#define TOGGLE_DEBOUNCE_MS 1000



/**ADVANCED CONFIGURATION**/
//There be dragons here.
#define FILENAME_SIZE sizeof(FILENAME_PREFIX) + sizeof(FILENAME_SUFFIX) + 5

#define SENSOR_ARR_SIZE 20

//Type of 
#define SD_TYPE SdExFat
#define FILE_TYPE ExFatFile

//Defines size of contiguous buffer that holds blocks. Basically one big array accessed in chunks.
#define BUFFER_SIZE SD_BLOCK_COUNT * SD_BLOCK_SIZE

#define SERIAL_TIMEOUT 1000

#define NRF_PA_LEVEL RF24_PA_LOW
#define NRF_CAR_ADDRESS "baja1"
/*=*=*=*SECTION COMPILE-TIME CHECKS*=*=*=*/


static_assert(NRF_BLOCK_SIZE <= 32, "NRF24l01 supports maximum packet sizes of 32 bytes!");
#endif