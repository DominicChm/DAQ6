#include "Sensor.h"
#include "config.h"

class SensorMarker : public Sensor {
private:
    int pin;
    bool last_state;
public:
    SensorMarker(uint8_t id, int pin) : Sensor(id) {
        this->pin = pin;
        pinMode(this->pin, INPUT_PULLUP);
        last_state = digitalRead(pin);
    };

    uint16_t readPacketBlock(uint8_t *buffer) override {
        bool state = digitalRead(pin);
        if (last_state != state) {
            buffer[0] = id;
            buffer[1] = !state;
            sensorPrint("Marker;\t");
            last_state = state;
            return 2;
        } else {
            return 0;
        }
    } //Writes a packet to the buffer, returns the size of data written.
    void start() override { ; }

    void stop() override { ; }

    void loop() override { ; }
};