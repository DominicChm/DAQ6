#include <Arduino.h>

#ifndef SENSOR_H
#define SENSOR_H

class Sensor
{
public:
    Sensor(/* args */) {};
    ~Sensor() {}

    virtual uint16_t readPacketBlock(uint8_t* buffer) = 0; //Writes a packet to the buffer, returns the size of data written.
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void loop() = 0;
};

#endif