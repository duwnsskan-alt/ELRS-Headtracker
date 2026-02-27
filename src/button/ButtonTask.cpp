#include "ButtonTask.h"
#include "sensor/Orientation.h"
#include "config/Config.h"

// ══════════════════════════════════════════════════════════════
//  태스크 시작
// ══════════════════════════════════════════════════════════════
void ButtonTask::start() {
    pinMode(PIN_BTN_CENTER, INPUT_PULLUP);
    xTaskCreate(
        taskFn,
        "ButtonTask",
        TASK_BUTTON_STACK,
        nullptr,
        TASK_BUTTON_PRIO,
        nullptr
    );
}

// ══════════════════════════════════════════════════════════════
//  FreeRTOS 태스크 함수  —  10ms 폴링 상태 머신
// ══════════════════════════════════════════════════════════════
void ButtonTask::taskFn(void* /*arg*/) {
    Serial.println("[BTN] Task started");

    State    state      = State::IDLE;
    uint32_t pressStart = 0;   // PRESSED 진입 시각 (ms)
    uint32_t releaseAt  = 0;   // WAIT_DOUBLE 진입 시각 (ms)

    while (true) {
        const uint32_t now     = millis();
        const bool     pressed = (digitalRead(PIN_BTN_CENTER) == LOW);

        switch (state) {

        // ── IDLE: 버튼 눌릴 때까지 대기 ──────────────────────
        case State::IDLE:
            if (pressed) {
                pressStart = now;
                state = State::PRESSED;
            }
            break;

        // ── PRESSED: 홀드 길이 측정 ───────────────────────────
        case State::PRESSED: {
            uint32_t hold = now - pressStart;

            if (!pressed) {
                // 릴리즈
                if (hold < BTN_DEBOUNCE_MS) {
                    // 노이즈 — 무시
                    state = State::IDLE;
                } else if (hold >= BTN_VERY_LONG_MS) {
                    fireGesture(Gesture::VERY_LONG_PRESS);
                    state = State::IDLE;
                } else if (hold >= BTN_LONG_MS) {
                    fireGesture(Gesture::LONG_PRESS);
                    state = State::IDLE;
                } else {
                    // 짧게 뗌 → 더블탭 대기
                    releaseAt = now;
                    state = State::WAIT_DOUBLE;
                }
            } else if (hold >= BTN_VERY_LONG_MS) {
                // 홀드 중 very long 달성 → 즉시 발화
                fireGesture(Gesture::VERY_LONG_PRESS);
                // 버튼 뗄 때까지 IDLE 전환 대기
                while (digitalRead(PIN_BTN_CENTER) == LOW) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                state = State::IDLE;
            }
            break;
        }

        // ── WAIT_DOUBLE: 더블탭 인터벌 대기 ─────────────────
        case State::WAIT_DOUBLE:
            if (pressed) {
                // 두 번째 탭 감지
                fireGesture(Gesture::DOUBLE_TAP);
                // 버튼 뗄 때까지 대기 (이번 릴리즈는 무시)
                while (digitalRead(PIN_BTN_CENTER) == LOW) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                state = State::IDLE;
            } else if (now - releaseAt >= BTN_DOUBLE_GAP_MS) {
                // 타임아웃 → 단탭 확정
                fireGesture(Gesture::SINGLE_TAP);
                state = State::IDLE;
            }
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ══════════════════════════════════════════════════════════════
//  제스처 → gState 동작
// ══════════════════════════════════════════════════════════════
void ButtonTask::fireGesture(Gesture g) {
    switch (g) {

    case Gesture::SINGLE_TAP:
        Orientation::resetCenter(gState);
        Serial.println("[BTN] Single tap → Center reset");
        break;

    case Gesture::DOUBLE_TAP:
        if (gState.lock()) {
            gState.tracking = !gState.tracking;
            bool t = gState.tracking;
            gState.unlock();
            Serial.printf("[BTN] Double tap → Tracking %s\n", t ? "ON" : "OFF");
        }
        break;

    case Gesture::LONG_PRESS:
        // Phase 2 확장 예약
        Serial.println("[BTN] Long press (reserved)");
        break;

    case Gesture::VERY_LONG_PRESS:
        if (gState.lock()) {
            gState.wifiApActive = true;
            gState.unlock();
        }
        Serial.println("[BTN] Very long press → WiFi AP (reserved)");
        break;
    }
}
