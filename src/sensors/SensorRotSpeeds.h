#include "Sensor.h"
#include "config.h"
#include "WheelSpeed.h"
#include "ChRt.h"
#include "macros.h"

class SensorRotSpeeds : public Sensor {
private:
    int pin{};
    const uint8_t id = 0x04;
    WheelSpeed *eWheelSpeed;
    WheelSpeed *rWheelsSpeed;
    WheelSpeed *flWheelSpeed;
    WheelSpeed *frWheelSpeed;
public:
    explicit SensorRotSpeeds(uint8_t id) : Sensor(id) {
        eWheelSpeed = new WheelSpeed(8);
        rWheelsSpeed = new WheelSpeed(24);
        flWheelSpeed = new WheelSpeed(24);
        frWheelSpeed = new WheelSpeed(24);
    };


    virtual uint16_t readPacketBlock(uint8_t *buffer) {
        static uint16_t ews = 0;
        static uint16_t rws = 0;
        static uint16_t flws = 0;
        static uint16_t frws = 0;

        uint16_t tews = ews;
        uint16_t trws = rws;
        uint16_t tflws = flws;
        uint16_t tfrws = frws;

        noInterrupts();
        ews = eWheelSpeed->read();
        rws = rWheelsSpeed->read();
        flws = flWheelSpeed->read();
        frws = frWheelSpeed->read();
        interrupts();

        if (ews != tews || rws != trws || flws != tflws || frws != tfrws || true) { //Only write if somthing's changed

            buffer[0] = id;

            buffer[1] = hiByte(ews);
            buffer[2] = loByte(ews);

            buffer[3] = hiByte(rws);
            buffer[4] = loByte(rws);

            buffer[5] = hiByte(flws);
            buffer[6] = loByte(flws);

            buffer[7] = hiByte(frws);
            buffer[8] = loByte(frws);


            sensorPrint("RotSpeeds: ");
            sensorPrint(ews);
            sensorPrint(", ");
            sensorPrint(rws);
            sensorPrint(", ");
            sensorPrint(flws);
            sensorPrint(", ");
            sensorPrint(frws);
            sensorPrint(";\t");

            sensorPrint(digitalRead(PIN_RSPEED_RGO));

            return 9;
        }

        return 0;

    } //Writes a packet to the buffer, returns the size of data written.

    void calcESpeed() { eWheelSpeed->calc(); }

    void calcRWheels() { rWheelsSpeed->calc(); }

    void calcFLWheel() { flWheelSpeed->calc(); }

    void calcFRWheel() { frWheelSpeed->calc(); }

    virtual void start() { ; }

    virtual void stop() { ; }

    virtual void loop() { ; }
};
