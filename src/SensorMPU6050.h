#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps_V6_12.h>
#include "Sensor.h"
#include "macros.h"
#include <Wire.h>

class SensorMPU6050 : public Sensor
{
private:
    bool dmpReadyFlag = false;
    float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
    Quaternion q;           // [w, x, y, z]         quaternion container
    VectorInt16 aa;         // [x, y, z]            accel sensor measurements
    VectorInt16 gy;         // [x, y, z]            gyro sensor measurements
    VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
    VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
    VectorFloat gravity;    // [x, y, z]            gravity vector
    float euler[3];         // [psi, theta, phi]    Euler angle container

    MPU6050 mpu;
    uint8_t devStatus;
    uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
    uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
    uint16_t fifoCount;     // count of all bytes currently in FIFO
    uint8_t fifoBuffer[64]; // FIFO storage buffer


    bool dmpReady = false;

public:
    SensorMPU6050() {
        Wire.begin();
        Wire.setClock(400000); 

        mpu.initialize();

        Serial.println(F("Testing device connections..."));
        Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

        devStatus = mpu.dmpInitialize();

        mpu.setXGyroOffset(51);
        mpu.setYGyroOffset(8);
        mpu.setZGyroOffset(21);
        mpu.setXAccelOffset(1150); 
        mpu.setYAccelOffset(-50); 
        mpu.setZAccelOffset(1060); 
        if(devStatus != 0) {
            debugl("MPU INIT ERROR :(");
            return;
        }

        mpu.CalibrateAccel(6);
        mpu.CalibrateGyro(6);

        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        mpuIntStatus = mpu.getIntStatus();
        
        
        mpuIntStatus = mpu.getIntStatus();

        packetSize = mpu.dmpGetFIFOPacketSize();
        dmpReady = true;

    }

    virtual uint16_t readPacketBlock(uint8_t* buffer){
        sensorPrint("Yaw: ");
        sensorPrint(ypr[0]);
        sensorPrint(" Pitch: ");
        sensorPrint(ypr[1]);
        sensorPrint(" Roll: ");
        sensorPrint(ypr[2]);
        sensorPrint("\t");
            
        
        return 0;
    }

    void dataReady() {
        dmpReadyFlag = true;
    }

    virtual void start(){;}
    virtual void stop(){;}
    virtual void loop(){
        if(dmpReady) {
            fifoCount = mpu.getFIFOCount();
            if(dmpReadyFlag || fifoCount > packetSize) {
                dmpReadyFlag = false;
                mpuIntStatus = mpu.getIntStatus();
                fifoCount = mpu.getFIFOCount();


                if ((mpuIntStatus & _BV(MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) || fifoCount >= 1024) {
                    // reset so we can continue cleanly
                    mpu.resetFIFO();
                    fifoCount = mpu.getFIFOCount();
                    Serial.println(F("FIFO overflow!"));

                    fifoCount = mpu.getFIFOCount();
                } else if (mpuIntStatus & _BV(MPU6050_INTERRUPT_DMP_INT_BIT)) {
                    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

                    mpu.getFIFOBytes(fifoBuffer, packetSize);

                    fifoCount -= packetSize;


                    mpu.dmpGetQuaternion(&q, fifoBuffer);
                    mpu.dmpGetGravity(&gravity, &q);
                    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
                }
            }
        }
        
    }
};