#include "Sensor.h"
#include "config.h"

class SensorMarker : public Sensor
{
private:
    int pin;
    const uint8_t id = 0x03;
public:
    SensorMarker(int pin) {
        this->pin = pin;
        pinMode(this->pin, INPUT_PULLUP);
    };

    virtual uint16_t readPacketBlock(uint8_t* buffer){
        if(digitalRead(pin)) {
            buffer[0] = id;
            buffer[1] = 0x01;
            debug("Marker;\t");
            return 2;
            
        } else {
            return 0;
        }
    } //Writes a packet to the buffer, returns the size of data written.
};