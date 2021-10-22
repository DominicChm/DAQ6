#include "Sensor.h"

class SensorTime : public Sensor {
private:
    uint32_t offset = 0;
public:
    explicit SensorTime(uint8_t id) : Sensor(id) {}

    uint16_t readPacketBlock(uint8_t *buffer) override {
        uint32_t time = millis() - offset;
        buffer[0] = 0x01;

        //fugly way to write buffer bytes 1-4 as a single uint32_t.
        *((uint32_t *) &buffer[1]) = time;

        sensorPrint("Time: ");
        sensorPrint(time);
        sensorPrint(";\t");

        return 5; //ID byte + 4 byte sensor reading
    } //Writes a packet to the buffer, returns the size of data written.

    void start() override {
        offset = millis();
    }

    void stop() override {}

    void loop() override {}
};