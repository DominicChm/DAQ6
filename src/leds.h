//
// Created by Domo2 on 10/20/2021.
//
#include "config.h"
#include "jled.h"

#ifndef DAQ6_LEDS_H
#define DAQ6_LEDS_H

JLed status_led = JLed(PIN_STATUS_LED);
JLed logger_led = JLed(PIN_LOGGER_LED);
JLed marker_led = JLed (PIN_MARKER_LED);

#endif //DAQ6_LEDS_H
