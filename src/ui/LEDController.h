#pragma once
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
//  LEDController  —  XIAO C6 내장 WS2812 RGB LED
//
//  상태별 색상/패턴:
//  ┌─────────────────┬────────────┬───────────────────────────┐
//  │ 상태             │ 색상        │ 패턴                       │
//  ├─────────────────┼────────────┼───────────────────────────┤
//  │ 부팅/초기화      │ 노랑        │ 느린 깜빡임 (500ms)        │
//  │ 센서 오류        │ 빨강        │ 빠른 깜빡임 (100ms)        │
//  │ ESP-NOW 연결됨   │ 초록        │ Solid                     │
//  │ ESP-NOW 탐색중   │ 초록        │ 빠른 깜빡임 (200ms)        │
//  │ SBUS 모드        │ 파랑        │ Solid                     │
//  │ WiFi AP 활성     │ 보라        │ Solid                     │
//  │ 바인드 모드      │ 하늘        │ 펄스 (숨쉬기)              │
//  │ 트래킹 OFF       │ 꺼짐        │ -                         │
//  │ 센터 리셋        │ 흰색        │ 짧은 플래시 (200ms 1회)    │
//  └─────────────────┴────────────┴───────────────────────────┘
// ═══════════════════════════════════════════════════════════════

enum class LEDState : uint8_t {
    OFF = 0,
    BOOTING,          // 노랑 깜빡임
    SENSOR_ERROR,     // 빨강 빠른 깜빡임
    ESPNOW_SEARCHING, // 초록 빠른 깜빡임
    ESPNOW_CONNECTED, // 초록 solid
    SBUS_ACTIVE,      // 파랑 solid
    WIFI_AP_ON,       // 보라 solid
    BINDING,          // 하늘 펄스
    FLASH_WHITE,      // 흰색 1회 플래시 (센터 리셋 피드백)
};

class LEDController {
public:
    static void init();
    static void setState(LEDState state);
    static void update();   // loop/태스크에서 주기적 호출

    // 센터 리셋 피드백 (흰색 플래시 1회)
    static void flashWhite();

private:
    // WS2812 단일 LED 색상 출력
    static void setColor(uint8_t r, uint8_t g, uint8_t b);
    static void off();

    static LEDState  _state;
    static uint32_t  _lastMs;
    static bool      _blinkOn;
    static uint8_t   _pulseVal;
    static int8_t    _pulseDir;

    // 플래시 상태
    static bool      _flashing;
    static uint32_t  _flashEnd;
    static LEDState  _prevState;  // 플래시 후 복귀할 상태
};
