#include <Arduino.h>

class DebouncedButton {
private:
    uint32_t next_trigger = 0;
    bool triggeredOn = LOW;
    uint8_t buttonPin;
    uint8_t pin_mode;
    uint32_t debounceMs;
    bool debounced{};
public:
    explicit DebouncedButton(uint8_t buttonPin, bool inverted = false, uint8_t pin_mode = INPUT_PULLUP,
                    uint32_t debounceMs = 1000) {
        this->buttonPin = buttonPin;
        this->pin_mode = pin_mode;
        this->debounceMs = debounceMs;


        if (pin_mode == INPUT_PULLUP)
            triggeredOn = inverted ? HIGH : LOW;
        else
            triggeredOn = inverted ? LOW : HIGH;


        pinMode(buttonPin, pin_mode);
        next_trigger = millis();
    };


    /*When polled, this function will return true once per button press cycle.*/
    bool isTriggered() { //TODO - Consider using an interrupt? Would make the lib less portable...
        debounced &= digitalRead(buttonPin) == triggeredOn; //Prevent multiple triggers without at least one state change.

        if ((millis() > next_trigger) && (digitalRead(buttonPin) == triggeredOn) && !debounced) {
            next_trigger = millis() + debounceMs;
            debounced = true;
            return true;
        } else return false;

    };
};