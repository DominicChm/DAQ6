#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"
#include "Sensor.h"
#include "macros.h"
#include <Wire.h>

class SensorMPU6050 : public Sensor {
private:
    volatile bool dmpReadyFlag = false;
    Quaternion q;           // [w, x, y, z]         quaternion container
    VectorInt16 aa;         // [x, y, z]            accel sensor measurements
    VectorInt16 gyro;     // [x, y, z]            Gyro measurements

    MPU6050 mpu;
    uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
    uint8_t fifoBuffer[64]{}; // FIFO storage buffer

    bool dmpReady = false;

public:
    explicit SensorMPU6050(uint8_t id) : Sensor(id) {
        /*Setup I2C for the MPU*/
        Wire1.begin();
        Wire1.setClock(400000);


        mpu = MPU6050(0x68, (void *) &Wire1);
        mpu.initialize();
        mpu.setIntDMPEnabled(true);
        mpu.setIntDataReadyEnabled(true);
        Serial.println(F("Testing device connections..."));
        Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

        int devStatus = mpu.dmpInitialize();

        /*Set offsets*/
        mpu.setXGyroOffset(51);
        mpu.setYGyroOffset(8);
        mpu.setZGyroOffset(21);
        mpu.setXAccelOffset(1150);
        mpu.setYAccelOffset(-50);
        mpu.setZAccelOffset(1060);
        if (devStatus != 0) {
            debugl("MPU INIT ERROR :(");
            return;
        }

        mpu.CalibrateAccel(6);
        mpu.CalibrateGyro(6);

        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);
        dmpReady = true;

        packetSize = mpu.dmpGetFIFOPacketSize();

    }

    uint16_t readPacketBlock(uint8_t *buffer) override {
        if (!dmpReady) return 0;

        sensorPrint("Ax: ");
        sensorPrint(aa.x);
        sensorPrint(" Ay: ");
        sensorPrint(aa.y);
        sensorPrint(" Roll: ");
        sensorPrint(aa.z);
        sensorPrint("\t");
        buffer[0] = id;

        //Save quaternion and acceleration - other values are derived (check mpu6050 lib for functions)
        //Define view of buffer to help with writing vars (i16bv = int 16 buffer view)
        auto *i16bv = (int16_t *) &buffer[1];
        i16bv[0] = aa.x;
        i16bv[1] = aa.y;
        i16bv[2] = aa.z;

        i16bv[3] = gyro.x;
        i16bv[4] = gyro.y;
        i16bv[5] = gyro.z;

        //New buffer view, at next empty index.
        auto *f32bv = (float *) &buffer[1 + sizeof(int16_t) * 6];
        f32bv[0] = q.x;
        f32bv[1] = q.y;
        f32bv[2] = q.z;
        f32bv[3] = q.w;

        return 1 + sizeof(int16_t) * 6 + sizeof(float) * 4;
    }

    void dataReady() {
        dmpReadyFlag = true;
    }

    void start() override {
    }

    void stop() override { ; }

    void loop() override {
        if (!dmpReady) return;
        if (!dmpReadyFlag) return;
        dmpReadyFlag = false;

        uint8_t mpuIntStatus = mpu.getIntStatus();
        uint16_t fifoCount = mpu.getFIFOCount();

        if ((mpuIntStatus & _BV(MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) || fifoCount >= 1024) {
            // reset so we can continue cleanly
            mpu.resetFIFO();
            Serial.println(F("FIFO overflow!"));

        } else if (mpuIntStatus & _BV(MPU6050_INTERRUPT_DMP_INT_BIT)) {
            //Wait until FIFO is full enough to read (if IRQ triggered)
            while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

            mpu.getFIFOBytes(fifoBuffer, packetSize);
            fifoCount -= packetSize;

            //Extract base values and log them.
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGyro(&gyro, fifoBuffer);
        }
    }
};