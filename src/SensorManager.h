//
// Created by Domo2 on 10/20/2021.
//
#include "Sensor.h"

#ifndef DAQ6_SENSORMANAGER_H
#define DAQ6_SENSORMANAGER_H

template<size_t sensor_capacity, size_t buffer_size>
class SensorManager {
private:
    int num_sensors = 0;
    Sensor *sensors[sensor_capacity]{};

public:
    uint8_t packet[buffer_size]{};
    size_t packet_size = 0;

    Sensor *add(Sensor *sensor) {
        sensors[num_sensors++] = sensor;
        return sensor;
    }

    /*Should be called at regular interval, by program code.*/
    void readSensors() {
        packet_size = 2; //Reserve 2 bytes in program code.
        for (Sensor *s: sensors)
            packet_size += s->readPacketBlock(&packet[packet_size]);
    }
};

#endif //DAQ6_SENSORMANAGER_H
