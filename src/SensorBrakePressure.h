#include "Sensor.h"
#include "config.h"
#include "WheelSpeed.h"
#include "ChRt.h"
#include "macros.h"

class SensorBrakePressures : public Sensor
{
private:
    int pin1, pin2;
    const uint8_t id = 0x04;
public:
    SensorBrakePressures(int pin1, int pin2) {
        pinMode(pin1, INPUT);
        pinMode(pin2, INPUT);
        this->pin1 = pin1;
        this->pin2 = pin2;
    };

    virtual uint16_t readPacketBlock(uint8_t* buffer){
        uint16_t p1 = analogRead(pin1);
        uint16_t p2 = analogRead(pin2);
        buffer[0] = id;
        buffer[1] = hiByte(p1);
        buffer[2] = lowByte(p1);
        buffer[3] = hiByte(p2);
        buffer[4] = lowByte(p2);
        return 5;
        
    } //Writes a packet to the buffer, returns the size of data written.
};
