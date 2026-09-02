#include "RelayManager.h"
#include "Config.h"

RelayManager::RelayManager(uint8_t pin)
    : pin_(pin) {
}

void RelayManager::begin() {
    pinMode(pin_, OUTPUT);
    off();
}

void RelayManager::on() {
    digitalWrite(pin_, RELAY_ON);
}

void RelayManager::off() {
    digitalWrite(pin_, RELAY_OFF);
}

bool RelayManager::isOn() const {
    return digitalRead(pin_) == RELAY_ON;
}
