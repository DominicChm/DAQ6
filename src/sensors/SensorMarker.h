#include "Sensor.h"
#include "config.h"

class SensorMarker : public Sensor {
private:
    int pin;
public:
    SensorMarker(uint8_t id, int pin) : Sensor(id) {
        this->pin = pin;
        pinMode(this->pin, INPUT_PULLUP);
    };

    uint16_t readPacketBlock(uint8_t *buffer) override {
        if (!digitalRead(pin)) {
            buffer[0] = id;
            buffer[1] = 0x01;
            sensorPrint("Marker;\t");
            return 2;

        } else {
            return 0;
        }
    } //Writes a packet to the buffer, returns the size of data written.
    void start() override { ; }

    void stop() override { ; }

    void loop() override { ; }
};