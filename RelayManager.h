#ifndef RELAY_MANAGER_H
#define RELAY_MANAGER_H

#include <Arduino.h>

class RelayManager {
public:
    explicit RelayManager(uint8_t pin);

    void begin();
    void on();
    void off();
    bool isOn() const;

private:
    uint8_t pin_;
};

#endif
