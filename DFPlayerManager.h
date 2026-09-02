#ifndef DFPLAYER_MANAGER_H
#define DFPLAYER_MANAGER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

class DFPlayerManager {
public:
    explicit DFPlayerManager(HardwareSerial& serial);

    bool begin();
    void setVolumePercent(int volumePercent);
    void handleFeedback();
    bool isPlaying();
    void playGreeting();
    void playRandomBackgroundMusic();
    void stop();
    bool consumeTrackFinished();

private:
    HardwareSerial& serial_;
    DFRobotDFPlayerMini player_;
    volatile bool trackFinished_ = false;
};

#endif
