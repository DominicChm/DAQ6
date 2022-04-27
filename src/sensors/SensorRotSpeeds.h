#include "Sensor.h"
#include "config.h"
#include "WheelSpeed.h"
#include "ChRt.h"
#include "macros.h"

class SensorRotSpeeds : public Sensor {
private:
    uint32_t ePulses{};
    uint32_t rWheelPulses{};
public:
    explicit SensorRotSpeeds(uint8_t id) : Sensor(id) {};

    uint16_t readPacketBlock(uint8_t *buffer) override {
        buffer[0] = id;

        noInterrupts();
        *((uint32_t *) &buffer[1]) = ePulses;
        *((uint32_t *) &buffer[5]) = rWheelPulses;
        interrupts();

        sensorPrint(ePulses);
        sensorPrint(";\tFRONT:");

        sensorPrint(rWheelPulses);
        sensorPrint(";\t");

        return 9;
    } //Writes a packet to the buffer, returns the size of data written.

    void ePulseISR() {
        ePulses++;
    }

    void rWheelPulseISR() {
        rWheelPulses++;
    }

    void start() override {
        ePulses = 0;
        rWheelPulses = 0;
    }

    void stop() override { ; }

    void loop() override { ; }
};
