#pragma once
#include <Arduino.h>
#include "SharedState.h"

// ═══════════════════════════════════════════════════════════════
//  ButtonTask  —  멀티탭 제스처 처리 (10ms 폴링 상태 머신)
//
//  제스처 → 동작:
//    Single tap   → 센터 리셋
//    Double tap   → 트래킹 on/off 토글
//    Long press   → (예약)
//    Very long    → WiFi AP 모드 (예약)
// ═══════════════════════════════════════════════════════════════

class ButtonTask {
public:
    // FreeRTOS 태스크 생성 (main.cpp에서 한 번 호출)
    static void start();

private:
    static void taskFn(void* arg);

    enum class State : uint8_t {
        IDLE,
        PRESSED,
        WAIT_DOUBLE,
    };

    // 확정된 제스처 종류
    enum class Gesture : uint8_t {
        SINGLE_TAP,
        DOUBLE_TAP,
        LONG_PRESS,
        VERY_LONG_PRESS,
    };

    // 제스처 발생 시 gState에 대한 실제 동작 수행
    static void fireGesture(Gesture g);
};
