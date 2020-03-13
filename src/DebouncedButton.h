#include <Arduino.h>

class DebouncedButton
{
private:
    uint32_t nextTriggerable = 0;
    bool triggeredOn     = LOW; 
    uint8_t buttonPin    ;
    uint8_t pinmode      ;
    uint32_t debounceMs  ;
public:
    DebouncedButton(uint8_t buttonPin, bool inverted = false, uint8_t pinmode = INPUT_PULLUP, uint32_t debounceMs = 1000) {
        this->buttonPin = buttonPin;
        this->pinmode = pinmode;
        this->debounceMs = debounceMs;

        switch (pinmode)
        {
            case INPUT_PULLUP:
                triggeredOn = inverted ? HIGH : LOW;
                break;
            case INPUT_PULLDOWN:
                triggeredOn = inverted ? LOW : HIGH;
                break;
        }


        pinMode(buttonPin, pinmode);
        nextTriggerable = millis();
    };


    /*When polled, this function will return true once per button press cycle.*/
    bool isTriggered() { //TODO - Consider using an interrupt? Would make the lib less portable...
        if( (millis() > nextTriggerable ) && (digitalRead(buttonPin) == triggeredOn) ) {
            nextTriggerable = millis() + debounceMs;
            return true;
        } else return false;
        
    };
};