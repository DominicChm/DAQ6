#include "Sensor.h"
#include "config.h"
#include "WheelSpeed.h"
#include "ChRt.h"
#include "macros.h"

class SensorBrakePressures : public Sensor {
private:
    int pin1, pin2;
public:
    SensorBrakePressures(uint8_t id, int pin1, int pin2) : Sensor(id) {
        pinMode(pin1, INPUT);
        pinMode(pin2, INPUT);
        this->pin1 = pin1;
        this->pin2 = pin2;
    };

    uint16_t readPacketBlock(uint8_t *buffer) override {
        uint16_t p1 = analogRead(pin1);
        uint16_t p2 = analogRead(pin2);

        buffer[0] = id;
        *((uint16_t *) &buffer[1]) = p1;
        *((uint16_t *) &buffer[3]) = p2;


        sensorPrint("Sensor Pressure: ");
        sensorPrint(p1);
        sensorPrint(", ");
        sensorPrint(p2);
        sensorPrint(";\t");

        return 5;

    } //Writes a packet to the buffer, returns the size of data written.

    void start() override {}
    void stop() override {}
    void loop() override {}
};
