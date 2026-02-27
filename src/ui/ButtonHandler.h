#pragma once
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
//  ButtonHandler  —  2버튼 이벤트 시스템
//
//  ┌────────────────────┬──────────────────────────────────────┐
//  │ 버튼 조합           │ 동작                                  │
//  ├────────────────────┼──────────────────────────────────────┤
//  │ CENTER 더블클릭     │ 트래킹 ON/OFF 토글                    │
//  │ CENTER 1초 홀드     │ 센터 리셋                             │
//  │ MODE   1초 홀드     │ 프로파일 순환 (1→2→3→1)              │
//  │ MODE   2초 홀드     │ 통신 모드 전환 (ESP-NOW ↔ SBUS)      │
//  │ CENTER + MODE 3초  │ WiFi AP 시작/종료                     │
//  └────────────────────┴──────────────────────────────────────┘
//
//  ⚠️ PIN_BTN_MODE: HW 도착 후 Config.h에서 확정 필요
// ═══════════════════════════════════════════════════════════════

enum class ButtonEvent : uint8_t {
    NONE = 0,
    CENTER_SHORT,        // 짧게 누름 (미사용, 확장용)
    CENTER_DOUBLE,       // 더블클릭 → 트래킹 토글
    CENTER_LONG,         // 1초 홀드 → 센터 리셋
    MODE_SHORT,          // 짧게 누름 (미사용, 확장용)
    MODE_LONG,           // 1초 홀드 → 프로파일 순환
    MODE_VERY_LONG,      // 2초 홀드 → 통신 모드 전환
    BOTH_VERY_LONG,      // CENTER+MODE 3초 → WiFi AP 토글
};

class ButtonHandler {
public:
    static void init();
    static void update();            // 20ms마다 호출 (ButtonTask)
    static ButtonEvent getEvent();   // 이벤트 큐에서 소비 (1회)

private:
    // 단일 버튼 상태 머신
    struct BtnState {
        uint8_t  pin;
        bool     pressed       = false;
        bool     prevRaw       = true;   // 풀업 기본 HIGH
        uint32_t pressMs       = 0;
        uint32_t releaseMs     = 0;
        uint8_t  clickCount    = 0;
        bool     longFired     = false;
        bool     veryLongFired = false;
    };

    static void processSingle(BtnState& btn);

    static BtnState  _center;
    static BtnState  _mode;
    static ButtonEvent _pendingEvent;

    // 양쪽 동시 홀드 감지
    static uint32_t _bothPressMs;
    static bool     _bothFired;
};
