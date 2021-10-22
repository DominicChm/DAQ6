#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"
#include "Sensor.h"
#include "macros.h"
#include <Wire.h>

class SensorMPU6050 : public Sensor {
private:
    volatile bool dmpReadyFlag = false;
    float ypr[3]{};           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
    VectorFloat gravity;    // [x, y, z]            gravity vector
    Quaternion q;           // [w, x, y, z]         quaternion container

    VectorInt16 aa;         // [x, y, z]            accel sensor measurements
    VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
    VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements

    MPU6050 mpu;
    uint8_t devStatus;
    uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
    uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
    uint16_t fifoCount{};     // count of all bytes currently in FIFO
    uint8_t fifoBuffer[64]{}; // FIFO storage buffer

    bool dmpReady = false;

public:
    explicit SensorMPU6050(uint8_t id) : Sensor(id) {
        /*Setup I2C for the MPU*/
        Wire1.begin();
        Wire1.setClock(400000);


        mpu = MPU6050(0x68, (void *) &Wire1);
        mpu.initialize();
        mpu.setRate(10);
        Serial.println(F("Testing device connections..."));
        Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

        devStatus = mpu.dmpInitialize();

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

        mpuIntStatus = mpu.getIntStatus();

        packetSize = mpu.dmpGetFIFOPacketSize();
        dmpReady = true;

    }

    virtual uint16_t readPacketBlock(uint8_t *buffer) {
        sensorPrint("Ax: ");
        sensorPrint(aa.x);
        sensorPrint(" Ay: ");
        sensorPrint(aa.y);
        sensorPrint(" Roll: ");
        sensorPrint(aa.z);
        sensorPrint("\t");

        sensorPrint("Yaw: ");
        sensorPrint(ypr[0]);
        sensorPrint(" Pitch: ");
        sensorPrint(ypr[1]);
        sensorPrint(" Roll: ");
        sensorPrint(ypr[2]);
        sensorPrint("\t");

        buffer[0] = id;

        //Save quaternion and acceleration - other values are derived (check mpu6050 lib for functions)
        //Define view of buffer to help with writing vars (i16bv = int 16 buffer view)
        auto *i16bv = (int16_t *) &buffer[1];
        i16bv[0] = aa.x;
        i16bv[1] = aa.y;
        i16bv[2] = aa.z;

        //New buffer view, at next empty index.
        auto *f32bv = (float *) &buffer[1 + sizeof(int16_t) * 3];
        f32bv[0] = q.x;
        f32bv[1] = q.y;
        f32bv[2] = q.z;
        f32bv[3] = q.w;

        return 1 + sizeof(int16_t) * 3 + sizeof(float) * 4;
    }

    void dataReady() {
        dmpReadyFlag = true;
        Serial.println("MPU DATA");
    }

    void start() override {
    }

    void stop() override { ; }

    void loop() override {
        if (dmpReady) {
            fifoCount = mpu.getFIFOCount();
            if (dmpReadyFlag || fifoCount > packetSize) {
                dmpReadyFlag = false;
                mpuIntStatus = mpu.getIntStatus();
                fifoCount = mpu.getFIFOCount();


                if ((mpuIntStatus & _BV(MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) || fifoCount >= 1024) {
                    // reset so we can continue cleanly
                    mpu.resetFIFO();
                    fifoCount = mpu.getFIFOCount();
                    Serial.println(F("FIFO overflow!"));

                } else if (mpuIntStatus & _BV(MPU6050_INTERRUPT_DMP_INT_BIT)) {
                    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

                    mpu.getFIFOBytes(fifoBuffer, packetSize);

                    fifoCount -= packetSize;


                    mpu.dmpGetQuaternion(&q, fifoBuffer);
                    mpu.dmpGetAccel(&aa, fifoBuffer);

                    mpu.dmpGetGravity(&gravity, &q);

                    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);

                    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);

                    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
                }
            }
        }

    }
};