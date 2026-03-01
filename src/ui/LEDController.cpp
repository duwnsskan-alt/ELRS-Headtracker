#include "LEDController.h"
#include "config/Config.h"
#include <Adafruit_NeoPixel.h>

// ── 정적 멤버 ─────────────────────────────────────────────────
LEDState LEDController::_state     = LEDState::BOOTING;
uint32_t LEDController::_lastMs    = 0;
bool     LEDController::_blinkOn   = false;
uint8_t  LEDController::_pulseVal  = 0;
int8_t   LEDController::_pulseDir  = 1;
bool     LEDController::_flashing  = false;
uint32_t LEDController::_flashEnd  = 0;
LEDState LEDController::_prevState = LEDState::OFF;

// NeoPixel 인스턴스 (XIAO C6 내장 RGB: 1개)
static Adafruit_NeoPixel pixel(1, PIN_LED, NEO_GRB + NEO_KHZ800);

// ══════════════════════════════════════════════════════════════
void LEDController::init() {
    pixel.begin();
    pixel.setBrightness(60);   // 0~255, 내장 LED는 밝아서 적당히 낮춤
    pixel.clear();
    pixel.show();
    _state = LEDState::BOOTING;
}

// ══════════════════════════════════════════════════════════════
void LEDController::setState(LEDState state) {
    if (_flashing) return;   // 플래시 중엔 무시
    _state = state;
}

// ══════════════════════════════════════════════════════════════
void LEDController::flashWhite() {
    _prevState = _state;
    _flashing  = true;
    _flashEnd  = millis() + 200;
    setColor(255, 255, 255);
}

// ══════════════════════════════════════════════════════════════
//  update() — LEDTask에서 20ms마다 호출
// ══════════════════════════════════════════════════════════════
void LEDController::update() {
    uint32_t now = millis();

    // ── 플래시 처리 (최우선) ─────────────────────────────────
    if (_flashing) {
        if (now >= _flashEnd) {
            _flashing = false;
            _state    = _prevState;
        } else {
            return;   // 플래시 중엔 아래 로직 건너뜀
        }
    }

    switch (_state) {

    // ── 꺼짐 ────────────────────────────────────────────────
    case LEDState::OFF:
        off();
        break;

    // ── 부팅: 노랑 500ms 깜빡임 ────────────────────────────
    case LEDState::BOOTING:
        if (now - _lastMs >= 500) {
            _lastMs  = now;
            _blinkOn = !_blinkOn;
            _blinkOn ? setColor(180, 120, 0) : off();
        }
        break;

    // ── 센서 오류: 빨강 100ms 빠른 깜빡임 ──────────────────
    case LEDState::SENSOR_ERROR:
        if (now - _lastMs >= 100) {
            _lastMs  = now;
            _blinkOn = !_blinkOn;
            _blinkOn ? setColor(255, 0, 0) : off();
        }
        break;

    // ── ESP-NOW 탐색: 초록 200ms 깜빡임 ────────────────────
    case LEDState::ESPNOW_SEARCHING:
        if (now - _lastMs >= 200) {
            _lastMs  = now;
            _blinkOn = !_blinkOn;
            _blinkOn ? setColor(0, 200, 0) : off();
        }
        break;

    // ── ESP-NOW 연결: 초록 solid ────────────────────────────
    case LEDState::ESPNOW_CONNECTED:
        setColor(0, 200, 0);
        break;

    // ── SBUS: 파랑 solid ────────────────────────────────────
    case LEDState::SBUS_ACTIVE:
        setColor(0, 80, 255);
        break;

    // ── WiFi AP: 보라 solid ─────────────────────────────────
    case LEDState::WIFI_AP_ON:
        setColor(120, 0, 180);
        break;

    // ── 바인드 모드: 하늘색 펄스 (숨쉬기) ──────────────────
    case LEDState::BINDING:
        if (now - _lastMs >= 15) {   // 15ms마다 밝기 변경
            _lastMs   = now;
            _pulseVal = (uint8_t)(_pulseVal + _pulseDir * 5);
            if (_pulseVal >= 200) { _pulseVal = 200; _pulseDir = -1; }
            if (_pulseVal <= 10)  { _pulseVal = 10;  _pulseDir =  1; }
            setColor(0, (uint8_t)(_pulseVal * 0.8f), _pulseVal);
        }
        break;

    // FLASH_WHITE는 flashWhite()에서 직접 처리
    case LEDState::FLASH_WHITE:
        break;
    }
}

// ══════════════════════════════════════════════════════════════
void LEDController::setColor(uint8_t r, uint8_t g, uint8_t b) {
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
}

void LEDController::off() {
    pixel.clear();
    pixel.show();
}
