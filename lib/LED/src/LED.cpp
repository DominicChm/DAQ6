#include <Arduino.h>
#include "LED.h"


//Major states 0-99
const int LED::UNCONTROLLED = 0;
const int LED::OFF = 1;
const int LED::ON = 2;
const int LED::CONSTANT_BLINK = 3;
const int LED::SOS_BLINK = 4;
const int LED::FAST_BLINK = 5;

//Advanced temp states (Require sequence) 100-199
const int LED::TEMP_BLINK = 100;

//Temp/control States 200-299
const int LED::WAITING_TO_TOGGLE = 200;
const int LED::WAITING_TO_OFF = 201;
const int LED::WAITING_TO_ON = 202;





LED::LED(int lpin)
{
    trk = new TreedStateTracker(10);
    pin = lpin;
    pinMode(pin, OUTPUT);
}

LED::~LED()
{

}

void LED::toggle() {
    setLED(!ledState);
}

void LED::setTimeout(long duration) {
    timeout = millis() + duration;
}

void LED::blink(long duration) {
    setState(TEMP_BLINK);
}

void LED::setLED(bool nstate){
    ledState = nstate;
    digitalWrite(pin, nstate);
}

void LED::setState(int nstate) {
    switch (nstate)
    {
        case 0 ... 99: //Major states
            reset();
        case 200 ... 299: //Temp states.
            trk->push(nstate);
            break; 
        default:
            break;
    }
    
}

void LED::setTimeoutState(int nstate, int timeout) {
    setTimeout(timeout);
    setState(nstate);
}

void LED::revertState() {
    trk->pop();
}

void LED::reset() {
    trk->reset();
    timeout = 0;
}

/*
const int LED::UNCONTROLLED = 0;
const int LED::OFF = 1;
const int LED::ON = 2;
const int LED::CONSTANT_BLINK = 3;
const int LED::SOS_BLINK = 4;
const int LED::FAST_BLINK = 5;
*/
void LED::tick() {
    switch (trk->current())
    {
    case UNCONTROLLED: // uncontrolled
        break;

    case ON:
        stateOn();
        break;

    case OFF:
        stateOff();
        break;

    case FAST_BLINK:
        stateFastBlink();   
        break; 

    case CONSTANT_BLINK: //1on, 1off blink
        stateConstantBlink();
        break; 

    case SOS_BLINK:
        stateSosBlink();
        break;

    case WAITING_TO_TOGGLE: //Temp state - Waiting for timeout to toggle.
        if(timedOut()) {
            toggle();
            revertState();
        }
        break;

    case WAITING_TO_ON: //Temp state - Waiting for timeout to set true 
        if(timedOut()) {
            setLED(true);
            revertState();
        }
        break;

    case WAITING_TO_OFF: //Temp state - waiting for timeout to set false; 
        if(timedOut()) {
            setLED(false);
            revertState();
        }
        break;

    case TEMP_BLINK: //Sequenced Temp state: blink
       stateTempBlink();
       break;
       
    default:
        break;
    }
}

bool LED::timedOut() {
    return millis() > timeout;
}

void LED::stateConstantBlink(){
    setTimeoutState(WAITING_TO_TOGGLE, 1000);
}

void LED::stateOn(){
    setLED(true);
}

void LED::stateOff(){
    setLED(false);
}

void LED::stateErrorBlink(){
    stateFastBlink();
}

void LED::stateSosBlink(){
    static int sequence = 0;
    

    switch (sequence) //Blink sequence test.
    {
        case 0 ... 5: //3 blinks
            setTimeoutState(WAITING_TO_TOGGLE, 150);
            break;
        case 6 ... 11:
            setTimeoutState(WAITING_TO_TOGGLE, 400);
            break;
        default:
            setLED(false);
            sequence = 0; //When func finish, this will become 0 B/C sequencestate++;
            return; //Return before
    }
    sequence++;
}

void LED::stateFastBlink(){
    setTimeoutState(WAITING_TO_TOGGLE, 100);
}



/*-----------------TEMP STATES---------------*/

void LED::stateTempBlink() {
    static int sequence = 0;

    switch (sequence) //Blink sequence - toggles LED twice.
    {
        case 0:
            setTimeoutState(WAITING_TO_TOGGLE, 500);
            break;
        case 1:
            setTimeoutState(WAITING_TO_TOGGLE, 500);
            break;
        default: 
            revertState();
            sequence = 0;
            return;
    }
    sequence++;
}

