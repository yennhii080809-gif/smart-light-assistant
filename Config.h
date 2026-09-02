#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// Hardware configuration - ESP32-C3 Super Mini
// ==========================================
#define PIR_PIN       1
#define RELAY_PIN     3
#define RELAY_ON      HIGH
#define RELAY_OFF     LOW

#define DFPLAYER_RX   20
#define DFPLAYER_TX   21
#define DFPLAYER_BAUDRATE 9600

// ==========================================
// MP3 configuration
// ==========================================
#define TRACK_VESINH         1
#define PLAYLIST_START_INDEX 2
#define PLAYLIST_TOTAL_SONGS 21

// ==========================================
// System timing
// ==========================================
#define WDT_TIMEOUT_SECONDS  8
#define AUTO_RESET_INTERVAL  86400000ULL
#define GREETING_COOLDOWN    30000

// ==========================================
// Default configuration
// ==========================================
#define DEFAULT_TIMEOUT_SECONDS 30
#define DEFAULT_VOLUME_PERCENT  80
#define DEBOUNCE_DELAY_MS       50

// Configuration limits
#define MIN_TIMEOUT_SECONDS     30
#define MAX_TIMEOUT_SECONDS     180
#define MIN_VOLUME_PERCENT      0
#define MAX_VOLUME_PERCENT      100

// ==========================================
// Wi-Fi Access Point
// ==========================================
#define AP_SSID      "ESP32C3_Smart_System"
#define AP_PASSWORD  "12345678"

#endif
