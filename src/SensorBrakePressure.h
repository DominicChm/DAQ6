#include "Sensor.h"
#include "config.h"
#include "WheelSpeed.h"
#include "ChRt.h"
#include "macros.h"

class SensorBrakePressure : public Sensor
{
private:
    int pin;
    const uint8_t id = 0x04;
public:
    SensorBrakePressure() {
        
    };

    virtual uint16_t readPacketBlock(uint8_t* buffer){
        static uint16_t ews  = 0;
        static uint16_t rws  = 0;
        static uint16_t flws = 0;
        static uint16_t frws = 0;

        uint16_t tews  = ews;
        uint16_t trws  = rws;
        uint16_t tflws = flws;
        uint16_t tfrws = frws;

        if(ews != tews || rws != trws || flws != tflws || frws != tfrws) { //Only write if somthing's changed
            buffer[0] = id;

            buffer[1] = hiByte(ews);
            buffer[2] = loByte(ews);

            buffer[3] = hiByte(rws);
            buffer[4] = loByte(rws);

            buffer[5] = hiByte(flws);
            buffer[6] = loByte(flws);

            buffer[7] = hiByte(frws);
            buffer[8] = loByte(frws);

            return 9;
        }

        return 0;
        
    } //Writes a packet to the buffer, returns the size of data written.
};
