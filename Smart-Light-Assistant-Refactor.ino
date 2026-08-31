#include "Config.h"
#include "Arduino.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "DFRobotDFPlayerMini.h"
#include <esp_task_wdt.h> // Thư viện Watchdog Timer

// ==========================================
// CẤU HÌNH PHẦN CỨNG CHO ESP32-C3 SUPER MINI
// ==========================================
#define PIR_PIN       1     // GPIO 1 
#define RELAY_PIN     3     // GPIO 3
#define RELAY_ON      HIGH
#define RELAY_OFF     LOW

#define DFPLAYER_RX   20     // GPIO 20 -> TX của DFPlayer
#define DFPLAYER_TX   21     // GPIO 21 -> RX của DFPlayer

// ==========================================
// CẤU HÌNH SỐ LƯỢNG FILE MP3 TRONG THƯ MỤC "mp3"
// ==========================================
#define TRACK_VESINH         1  // File 0001.mp3: Nhắc nhở giữ vệ sinh
#define PLAYLIST_START_INDEX 2  // Playlist nhạc nền bắt đầu từ file 0002.mp3
#define PLAYLIST_TOTAL_SONGS 21 // Tổng cộng có 21 bài nhạc nền (đến file 0022.mp3)

// Định nghĩa thời gian Watchdog (Hệ thống treo quá 8 giây sẽ tự reset)
#define WDT_TIMEOUT_SECONDS 8 

// Định nghĩa chu kỳ tự động Reset (24 giờ = 86400000 ms)
#define AUTO_RESET_INTERVAL 86400000ULL 

// ==========================================
// KHỞI TẠO ĐỐI TƯỢNG VÀ BIẾN TOÀN CỤC
// ==========================================
WebServer server(80);
Preferences preferences;
HardwareSerial SerialData(1); // Sử dụng UART1 của ESP32-C3
DFRobotDFPlayerMini myDFPlayer;
volatile bool trackFinished = false;

const char* apSSID = "ESP32C3_Smart_System";
const char* apPassword = "12345678";

unsigned long configTimeoutMs = 30000; // Mặc định ban đầu 30 giây
int currentVolumePercent = 80;         

unsigned long lastMotionTime = 0;
unsigned long lastGreetingTime = 0;
unsigned long lastResetTime = 0; // Quản lý mốc thời gian reset 24h

// Các biến phục vụ việc chống nhiễu PIR (Debounce)
bool lastPIRState = LOW;
bool stablePIRState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // 50ms ổn định trạng thái

#define GREETING_COOLDOWN 30000 // 30 giây chống lặp lời nhắc giữ vệ sinh

// Cập nhật lại các trạng thái logic mới
enum SystemState { IDLE, PLAYING_GREETING, PLAYING_MUSIC };
SystemState currentState = IDLE;

// ==========================================
// GIAO DIỆN WEB GLASSMORPHISM TỐI ƯU (30s - 3 Phút)
// ==========================================
const char HTML_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Cấu Hình Hệ Thống Thông Minh</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
        body {
            background: linear-gradient(135deg, #0f2027, #203a43, #2c5364);
            color: #fff;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            background: rgba(255, 255, 255, 0.06);
            backdrop-filter: blur(10px);
            -webkit-backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 16px;
            padding: 30px;
            width: 100%;
            max-width: 450px;
            box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
            text-align: center;
        }
        h2 { margin-bottom: 15px; font-weight: 600; font-size: 26px; color: #00f2fe; text-shadow: 0 0 10px rgba(0,242,254,0.3); }
        .status-wrapper { font-size: 20px; color: #e2e8f0; margin-bottom: 25px; }
        .status { font-weight: bold; padding: 6px 14px; border-radius: 8px; font-size: 22px; margin-left: 5px; display: inline-block; }
        .on { background: rgba(16, 185, 129, 0.3); color: #10b981; border: 1px solid #10b981; box-shadow: 0 0 10px rgba(16, 185, 129, 0.5); }
        .off { background: rgba(239, 68, 68, 0.3); color: #ef4444; border: 1px solid #ef4444; box-shadow: 0 0 10px rgba(239, 68, 68, 0.5); }
        .form-group { text-align: left; margin-bottom: 25px; }
        label { display: block; margin-bottom: 10px; font-size: 18px; color: #cbd5e1; }
        .range-wrapper { display: flex; align-items: center; justify-content: space-between; gap: 15px; }
        input[type="range"] { flex: 1; accent-color: #00f2fe; height: 8px; cursor: pointer; }
        .val-display { font-weight: bold; color: #00f2fe; min-width: 85px; text-align: right; font-size: 18px; }
        button {
            width: 100%;
            padding: 14px;
            background: linear-gradient(45deg, #0093e9, #80d0c7);
            border: none;
            border-radius: 8px;
            color: #fff;
            font-size: 18px;
            font-weight: bold;
            cursor: pointer;
            transition: 0.3s;
            box-shadow: 0 4px 15px rgba(0, 147, 233, 0.4);
            margin-top: 10px;
        }
        button:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(0, 147, 233, 0.6); }
        .alert-box { margin-top: 15px; font-size: 16px; padding: 10px; border-radius: 6px; background: rgba(0, 255, 135, 0.1); color: #00ff87; border: 1px solid #00ff87; display: none; }
        .footer { margin-top: 30px; font-size: 16px; color: #94a3b8; border-top: 1px solid rgba(255,255,255,0.1); padding-top: 15px; }
        .tech-support { color: #f59e0b; font-weight: bold; font-size: 18px; margin-top: 5px; letter-spacing: 0.5px; }
    </style>
</head>
<body>
    <div class="container">
        <h2>💡 Smart System Control</h2>
        <p class="status-wrapper">Trạng thái thiết bị: <span id="deviceStatus" class="status off">ĐANG TẮT</span></p>
        
        <form id="configForm">
            <div class="form-group">
                <label>Thời gian chờ tắt thiết bị:</label>
                <div class="range-wrapper">
                    <input type="range" id="timeout" name="timeout" min="30" max="180" step="5" value="30" oninput="document.getElementById('tOutVal').innerText = this.value + ' Giây'">
                    <span id="tOutVal" class="val-display">30 Giây</span>
                </div>
            </div>
            <div class="form-group">
                <label>Âm lượng loa thông báo:</label>
                <div class="range-wrapper">
                    <input type="range" id="volume" name="volume" min="0" max="100" value="80" oninput="document.getElementById('volVal').innerText = this.value + '%'">
                    <span id="volVal" class="val-display">80%</span>
                </div>
            </div>
            <button type="submit">Cập Nhật Cấu Hình</button>
        </form>
        <div id="alertBox" class="alert-box">Cập nhật thành công!</div>

        <div class="footer">
            <p>Hỗ trợ kỹ thuật:</p>
            <p class="tech-support">0332514109 - 0357002021</p>
        </div>
    </div>

    <script>
        window.onload = function() {
            fetch('/get-config')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('timeout').value = data.timeout;
                    document.getElementById('tOutVal').innerText = data.timeout + ' Giây';
                    document.getElementById('volume').value = data.volume;
                    document.getElementById('volVal').innerText = data.volume + '%';
                    
                    const statusBox = document.getElementById('deviceStatus');
                    if(data.relayState) {
                        statusBox.innerText = "ĐANG BẬT";
                        statusBox.className = "status on";
                    } else {
                        statusBox.innerText = "ĐANG TẮT";
                        statusBox.className = "status off";
                    }
                });
        };

        document.getElementById('configForm').addEventListener('submit', function(e) {
            e.preventDefault();
            const timeout = document.getElementById('timeout').value;
            const volume = document.getElementById('volume').value;

            fetch(`/set-config?timeout=${timeout}&volume=${volume}`)
                .then(response => response.text())
                .then(data => {
                    const alertBox = document.getElementById('alertBox');
                    alertBox.style.display = 'block';
                    alertBox.innerText = data;
                    setTimeout(() => { alertBox.style.display = 'none'; }, 3000);
                });
        });
    </script>
</body>
</html>
)rawliteral";

void applyVolume(int volumePercent) {
    int dfVolume = map(volumePercent, 0, 100, 0, 30);
    myDFPlayer.volume(dfVolume);
}

void handleRoot() {
    server.send_P(200, "text/html", HTML_INDEX);
}

void handleGetConfig() {
    bool rState = (digitalRead(RELAY_PIN) == RELAY_ON);
    char jsonBuffer[128];
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"timeout\":%lu,\"volume\":%d,\"relayState\":%s}", 
             configTimeoutMs / 1000, currentVolumePercent, rState ? "true" : "false");
             
    server.send(200, "application/json", jsonBuffer);
}

void handleSetConfig() {
    if (server.hasArg("timeout") && server.hasArg("volume")) {
        int t = server.arg("timeout").toInt(); 
        int v = server.arg("volume").toInt();

        if (t >= 30 && t <= 180 && v >= 0 && v <= 100) {
            configTimeoutMs = (unsigned long)t * 1000; 
            currentVolumePercent = v;

            preferences.begin("sys-config", false);
            preferences.putInt("timeout", t); 
            preferences.putInt("volume", v);
            preferences.end();

            applyVolume(currentVolumePercent);

            Serial.printf("[SETTINGS] Cập nhật -> Chờ: %d giây | Vol: %d%%\n", t, v);
            server.send(200, "text/plain", "Đã lưu cấu hình mới thành công!");
            return;
        }
    }
    server.send(400, "text/plain", "Dữ liệu cấu hình không hợp lệ!");
}

void checkDFPlayerFeedback() {
    if (myDFPlayer.available()) {
        uint8_t type = myDFPlayer.readType();
        int value = myDFPlayer.read();

        switch (type) {

        case DFPlayerPlayFinished:
            Serial.printf("[DFPLAYER] File %04d phát xong\n", value);
            trackFinished = true;
            break;

        case DFPlayerError:
            Serial.printf("[DFPLAYER ERROR] Mã lỗi: %d\n", value);
            break;
        }
    }
}

// Hàm kiểm tra module có đang phát nhạc hay không
bool isMusicPlaying() {
    int state = myDFPlayer.readState();

    Serial.printf("[DFPLAYER] State = %d\n", state);

    return (state == 512 || state == 513 || state == 1);
}

// Hàm hỗ trợ bốc bài hát ngẫu nhiên từ Playlist
void playRandomBackgroundMusic() {
    int randomTrack = random(PLAYLIST_START_INDEX, PLAYLIST_START_INDEX + PLAYLIST_TOTAL_SONGS); 
    Serial.printf("[FSM] Tiến hành phát nhạc nền ngẫu nhiên file số: %04d.mp3\n", randomTrack);
    myDFPlayer.playMp3Folder(randomTrack);
}

void setup() {
    Serial.begin(115200);
    SerialData.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);

    pinMode(PIR_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_OFF);

    preferences.begin("sys-config", true);
    int savedSeconds = preferences.getInt("timeout", 30);  
    int savedVolume = preferences.getInt("volume", 80);   
    preferences.end();

    configTimeoutMs = (unsigned long)savedSeconds * 1000;
    currentVolumePercent = savedVolume;

    if (!WiFi.softAP(apSSID, apPassword)) {
        Serial.println(F("[WiFi] Không thể khởi tạo Access Point!"));
    } else {
        Serial.print("[WiFi] AP Ready. IP Web: ");
        Serial.println(WiFi.softAPIP());
    }

    server.on("/", handleRoot);
    server.on("/get-config", handleGetConfig);
    server.on("/set-config", handleSetConfig);
    server.begin();

    delay(1000); 
    if (!myDFPlayer.begin(SerialData)) {
        Serial.println(F("[DFPLAYER] Không tìm thấy module hoặc thẻ nhớ!"));
    } else {
        applyVolume(currentVolumePercent);
        Serial.println(F("[DFPLAYER] Sẵn sàng đọc file từ thư mục mp3."));
    }
    
    randomSeed(esp_random());

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WDT_TIMEOUT_SECONDS * 1000,
        .idle_core_mask = (1 << 0), 
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdt_config);
#else
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true); 
#endif
    esp_task_wdt_add(NULL); 
    Serial.println(F("[SYSTEM] Khởi động Watchdog thành công!"));
    
    lastResetTime = millis();
    lastGreetingTime = millis() - GREETING_COOLDOWN;
}

void loop() {
    esp_task_wdt_reset(); 
    server.handleClient(); 
    checkDFPlayerFeedback(); 

    unsigned long now = millis();

    if (now - lastResetTime >= AUTO_RESET_INTERVAL) {
        Serial.println(F("[SYSTEM] Đã đến chu kỳ 24h hoạt động. Tự động Reset thiết bị..."));
        delay(1000); 
        esp_restart();
    }

    // --- THUẬT TOÁN CHỐNG NHIỄU (DEBOUNCE) CHO PIR ---
    bool reading = (digitalRead(PIR_PIN) == HIGH);
    if (reading != lastPIRState) {
        lastDebounceTime = now;
    }
    if ((now - lastDebounceTime) > debounceDelay) {
        if (reading != stablePIRState) {
            stablePIRState = reading;
        }
    }
    lastPIRState = reading;
    
    bool motionDetected = stablePIRState;

    switch (currentState) {
        
        case IDLE:
            if (motionDetected) {
                Serial.println(F("[FSM] Có người vào -> Bật Relay & Phát file nhắc vệ sinh."));
                digitalWrite(RELAY_PIN, RELAY_ON);
                lastMotionTime = now;

                if (now - lastGreetingTime >= GREETING_COOLDOWN) {
                    Serial.println("PHAT LOI NHAC >>> PLAY 0001");
                    myDFPlayer.playMp3Folder(TRACK_VESINH);
                    delay(300);
                    lastGreetingTime = now;
                    currentState = PLAYING_GREETING;
                } else {
                    Serial.println("PHAT NHAC >>> PLAY RANDOM");
                    playRandomBackgroundMusic();
                    currentState = PLAYING_MUSIC;
                }
            }
            break;

        case PLAYING_GREETING:
            if (motionDetected) {
                lastMotionTime = now; // Cập nhật liên tục khi có người di chuyển
            }

            // Kiểm tra xem lời nhắc vệ sinh đã phát xong chưa
            if (trackFinished) {
                trackFinished = false;
                Serial.println(F("[FSM] Lời nhắc phát xong -> Chuyển sang nhạc nền"));
                delay(150);   // Cho DFPlayer ổn định
                playRandomBackgroundMusic();
                currentState = PLAYING_MUSIC;
            }
            break;

        case PLAYING_MUSIC:
            if (motionDetected) {
                lastMotionTime = now; // Nếu vẫn có người di chuyển, reset mốc đếm thời gian timeout
            }

            // ĐIỀU KIỆN 1: HẾT THỜI GIAN CHỜ (Cường độ di chuyển bằng 0 sau khoảng configTimeoutMs)
            // Sẽ TẮT NGAY LẬP TỨC bất kể bài hát đã hết hay chưa
            if (now - lastMotionTime >= configTimeoutMs) {
                Serial.println(F("[FSM] Hết thời gian chờ thiết lập -> Ngắt Relay, dừng nhạc và về IDLE."));
                myDFPlayer.stop();
                trackFinished = false; // Xóa cờ thừa nếu có
                digitalWrite(RELAY_PIN, RELAY_OFF);
                currentState = IDLE;
            }
            // ĐIỀU KIỆN 2: NẾU HẾT BÀI NHƯNG VẪN TRONG THỜI GIAN CHO PHÉP (Vẫn còn người)
            // Thì tự động gối tiếp bài mới
            else if (trackFinished) {
                trackFinished = false; // Reset cờ để chuẩn bị bài mới
                Serial.println(F("[FSM] Hết bài nhưng vẫn trong thời gian chờ -> Chuyển bài ngẫu nhiên tiếp theo."));
                delay(150); 
                playRandomBackgroundMusic();
            }
            break;
    }
}
