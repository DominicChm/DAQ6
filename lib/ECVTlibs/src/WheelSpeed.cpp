/*
 *	WheelSpeed.cpp - Library for measuring wheel speed.
 *	Created by Rahul Goyal, July 1 2019.
 *	Released to Cal Poly Baja SAE. ;)
 */

#include <Arduino.h>
#include "WheelSpeed.h"

const uint32_t TIMEOUT = 1000000;

// Constructor
/** This constructor accepts the number of triggers per revolution and stores
	it. prevTime and currTime are initialized to the system time. **/
WheelSpeed::WheelSpeed(int8_t triggers) {
	// Initialize variables
	this->triggers = triggers;
	prevTime = micros();
	currTime = micros();
}

// Interrupt Service Routine Method
/** This function stores the value of currTime (from the previous call to this
	function by the interrupt service routine) in prevTime and currTime is
	updated to the system time. This information is used by the read method to
	calculate the wheel speed. **/
void WheelSpeed::calc() {
	prevTime = currTime;
	currTime = micros();
}

// Read Wheel Speed Method
/** This function uses the value of currTime and the value of prevTime to
	calculate the wheel speed and return it as an 16-bit integer. If the time
	difference exceeds the defined TIMEOUT constant (in microseconds), a value
	of zero is returned to indicate that wheel is not moving. **/
int16_t WheelSpeed::read() {
	if (micros() - prevTime >= TIMEOUT) {
		return 0;
	}
	// return (float)1000000 / ((currTime - prevTime) * triggers);		// Revolutions per Second (RPS)
	return (float)60000000 / ((currTime - prevTime) * triggers);	// Revolutions per Minute (RPM)
}