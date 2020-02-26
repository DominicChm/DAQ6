/*DEBUG CONFIG*/
#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG

/*BUFFER CONFIG*/
#define BLOCK_SIZE 512


#define BLOCK_COUNT 10
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
#define LEDDRV_PRIORITY NORMALPRIO      + 10 
#define CONTRL_PRIORITY WRITER_PRIORITY + 10
#define WRITER_PRIORITY LEDDRV_PRIORITY + 10
#define READER_PRIORITY CONTRL_PRIORITY + 10

//Interval between LED driver thread runs in ms.
#define LEDTHD_INTERVAL 50

#define CONTROLTHD_INTERVAL 100


/***PIN CONFIG***/
    //Pin config for logger control - toggling and status.
        #define PIN_LOGGER_LED 4
        #define PIN_LOGGER_BTN 2
    
    //Pin for auxillary status LED - 13 is internal Teensy LED
        #define PIN_STATUS_LED 13

    //Marker button pin. 
        #define PIN_MARKER_BTN 5
    
    //Pins for the wheelspeed sensors. These are just hall effect sensors.
    //ENG = engine, RGO = Rear Gearbox Output, WFL = Wheel Front Left, WFR = Wheel Front Right
        #define PIN_RSPEED_ENG  5
        #define PIN_RSPEED_RGO  6
        #define PIN_RSPEED_WFL 29
        #define PIN_RSPEED_WFR 30
    
    //Brake Pressure transducer pins
        #define PIN_BPRESSURE_F 34
        #define PIN_BPRESSURE_R 33



/*SENSOR CONFIG*/
#define SAMPLE_INTERVAL 1

//Comment a define to disable a sensor.
#define SENSOR_TIME

#define SENSOR_MARKER

#define SENSOR_WHEELSPEED


/*CONTROLS CONFIG*/
#define TOGGLE_DEBOUNCE_MS 1000



/**ADVANCED CONFIGURATION**/
//There be dragons here.
#define MAX_BUFFER_SIZE 192 * 1024

#define FILENAME_SIZE sizeof(FILENAME_PREFIX) + sizeof(FILENAME_SUFFIX) + 5

#define SENSOR_ARR_SIZE 20

#define ABS_BUFFER_SIZE BUFFER_SIZE + MAX_PACKET_SIZE

#define SD_TYPE SdExFat
#define FILE_TYPE ExFatFile

#define BUFFER_SIZE BLOCK_COUNT * BLOCK_SIZE
/*This defines how many blocks there are. To simplify block tracking behavior, */
#define OVERFLOW_BIT 8
/*=*=*=*SECTION COMPILE-TIME CHECKS*=*=*=*/



#endif