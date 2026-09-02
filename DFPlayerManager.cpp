#include "DFPlayerManager.h"
#include "Config.h"

DFPlayerManager::DFPlayerManager(HardwareSerial& serial)
    : serial_(serial) {
}

bool DFPlayerManager::begin() {
    serial_.begin(DFPLAYER_BAUDRATE, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
    return player_.begin(serial_);
}

void DFPlayerManager::setVolumePercent(int volumePercent) {
    int dfVolume = map(volumePercent, 0, 100, 0, 30);
    player_.volume(dfVolume);
}

void DFPlayerManager::handleFeedback() {
    if (player_.available()) {
        uint8_t type = player_.readType();
        int value = player_.read();

        switch (type) {
        case DFPlayerPlayFinished:
            Serial.printf("[DFPLAYER] File %04d phat xong\n", value);
            trackFinished_ = true;
            break;

        case DFPlayerError:
            Serial.printf("[DFPLAYER ERROR] Ma loi: %d\n", value);
            break;
        }
    }
}

bool DFPlayerManager::isPlaying() {
    int state = player_.readState();

    Serial.printf("[DFPLAYER] State = %d\n", state);

    return (state == 512 || state == 513 || state == 1);
}

void DFPlayerManager::playGreeting() {
    player_.playMp3Folder(TRACK_VESINH);
}

void DFPlayerManager::playRandomBackgroundMusic() {
    int randomTrack = random(PLAYLIST_START_INDEX,
                            PLAYLIST_START_INDEX + PLAYLIST_TOTAL_SONGS);
    Serial.printf("[FSM] Tien hanh phat nhac nen ngau nhien file so: %04d.mp3\n", randomTrack);
    player_.playMp3Folder(randomTrack);
}

void DFPlayerManager::stop() {
    player_.stop();
}

bool DFPlayerManager::consumeTrackFinished() {
    if (!trackFinished_) {
        return false;
    }

    trackFinished_ = false;
    return true;
}
