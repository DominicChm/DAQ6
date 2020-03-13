#include "Sensor.h"
#include "config.h"

/*PACKET FORMAT*/
/*
PREFACED BY (2x) 01010101
int16_t eSpeed;
int32_t pEnc;
int32_t sEnc;

int8_t eState;
int8_t pState;
int8_t sState;

int16_t ePID;
int16_t pPID;
int16_t sPID;

int16_t pSetpoint;
int16_t sSetpoint;

int16_t eP; 
int16_t eI;
int16_t eD;

uint16_t checksum;

*/
class SensorECVT : public Sensor
{
private:
    const byte id = 0x03;
public:
    SensorECVT() {
        //pinMode(MARKER_BTN, INPUT_PULLUP);
    };

    virtual uint16_t readPacketBlock(uint8_t* buffer) {/*
        if(!digitalRead(MARKER_BTN)) {
            buffer[0] = id;
            buffer[1] = 0x01;
            return 2;
        } else {
            return 0;
        }
        */
       return 0;
    } //Writes a packet to the buffer, returns the size of data written.
        virtual void start(){;}
    virtual void stop(){;}
    virtual void loop(){;}
};