#include "TreedStateTracker.h"

#ifndef LED_H
#define LED_H
class LED
{
private:
    bool ledState = 0;

    uint32_t timeout = 0;
    int pin;

    TreedStateTracker *trk;


    /*State Functions*/
    void stateConstantBlink();
    void stateOn();
    void stateOff();
    void stateErrorBlink();
    void stateSosBlink();
    void stateFastBlink();


    void stateTempBlink();
    void setTimeoutState(int sta, int timeout);
    bool timedOut();
public:
    LED(int lpin);
    ~LED();

    void toggle();
    void setLED(bool ledState);
    void tick();
    void blink(long duration);
    void setTimeout(long duration);
    void revertState();
    void reset();
    void setState(int state);

    

    //State Consts//
    static const int UNCONTROLLED;
    static const int OFF;
    static const int ON;
    static const int CONSTANT_BLINK;
    static const int SOS_BLINK;
    static const int FAST_BLINK;

    //Temp States
    static const int WAITING_TO_TOGGLE; 
    static const int WAITING_TO_OFF;
    static const int WAITING_TO_ON;
    static const int TEMP_BLINK;

};

#endif