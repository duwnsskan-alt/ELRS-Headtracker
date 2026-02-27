#include "ButtonHandler.h"
#include "config/Config.h"

// ── 정적 멤버 ─────────────────────────────────────────────────
ButtonHandler::BtnState ButtonHandler::_center = {PIN_BTN_CENTER};
ButtonHandler::BtnState ButtonHandler::_mode   = {PIN_BTN_MODE};
ButtonEvent             ButtonHandler::_pendingEvent = ButtonEvent::NONE;
uint32_t                ButtonHandler::_bothPressMs  = 0;
bool                    ButtonHandler::_bothFired    = false;

// ══════════════════════════════════════════════════════════════
void ButtonHandler::init() {
    pinMode(PIN_BTN_CENTER, INPUT_PULLUP);
    // ⚠️ PIN_BTN_MODE HW 미확정 — 핀 번호 확정 후 활성화
    // pinMode(PIN_BTN_MODE, INPUT_PULLUP);
    Serial.printf("[BTN] Init — CENTER=GPIO%d  MODE=GPIO%d(TBD)\n",
                  PIN_BTN_CENTER, PIN_BTN_MODE);
}

// ══════════════════════════════════════════════════════════════
//  update() — 20ms 주기로 호출
// ══════════════════════════════════════════════════════════════
void ButtonHandler::update() {
    uint32_t now = millis();

    // ── 양쪽 동시 홀드 감지 (CENTER+MODE 3초) ────────────────
    bool cPressed = (digitalRead(PIN_BTN_CENTER) == LOW);
    // bool mPressed = (digitalRead(PIN_BTN_MODE) == LOW);  // HW 미확정
    bool mPressed = false;  // TODO: HW 확정 후 윗줄로 교체

    if (cPressed && mPressed) {
        if (_bothPressMs == 0) _bothPressMs = now;
        if (!_bothFired && (now - _bothPressMs >= BTN_VERY_LONG_MS)) {
            _bothFired    = true;
            _pendingEvent = ButtonEvent::BOTH_VERY_LONG;
        }
    } else {
        _bothPressMs = 0;
        _bothFired   = false;
    }

    // 양쪽 동시 입력 중엔 개별 처리 생략
    if (cPressed && mPressed) return;

    // ── 개별 버튼 처리 ───────────────────────────────────────
    processSingle(_center);
    processSingle(_mode);
}

// ══════════════════════════════════════════════════════════════
//  단일 버튼 상태 머신
// ══════════════════════════════════════════════════════════════
void ButtonHandler::processSingle(BtnState& btn) {
    uint32_t now = millis();
    bool raw = (digitalRead(btn.pin) == LOW);   // LOW = 눌림 (풀업)

    // ── 디바운스 ─────────────────────────────────────────────
    if (raw == btn.prevRaw) {
        btn.prevRaw = raw;
    }

    bool justPressed  = (!btn.pressed && raw);
    bool justReleased = ( btn.pressed && !raw);
    btn.prevRaw = raw;

    // ── 눌림 시작 ────────────────────────────────────────────
    if (justPressed) {
        btn.pressed       = true;
        btn.pressMs       = now;
        btn.longFired     = false;
        btn.veryLongFired = false;
    }

    // ── 홀드 중 long / very_long 발화 ────────────────────────
    if (btn.pressed) {
        uint32_t held = now - btn.pressMs;

        bool isModeBtn = (btn.pin == PIN_BTN_MODE);

        if (!btn.veryLongFired && isModeBtn && held >= BTN_LONG_MS * 2) {
            // MODE 2초 = 통신 모드 전환
            btn.veryLongFired = true;
            btn.longFired     = true;   // long도 소비됨
            _pendingEvent = ButtonEvent::MODE_VERY_LONG;
        }
        else if (!btn.longFired && held >= BTN_LONG_MS) {
            btn.longFired = true;
            if (!isModeBtn) {
                _pendingEvent = ButtonEvent::CENTER_LONG;  // 센터 리셋
            } else {
                _pendingEvent = ButtonEvent::MODE_LONG;    // 프로파일 순환
            }
        }
    }

    // ── 릴리즈 ───────────────────────────────────────────────
    if (justReleased) {
        uint32_t held = now - btn.pressMs;
        btn.pressed   = false;

        bool isModeBtn = (btn.pin == PIN_BTN_MODE);

        // long/very_long은 눌리는 중에 이미 발화됨 → 릴리즈 무시
        if (btn.longFired) {
            btn.clickCount = 0;
            btn.releaseMs  = now;
            return;
        }

        // short 클릭 누적
        if (held >= BTN_DEBOUNCE_MS) {
            btn.clickCount++;
            btn.releaseMs = now;
        }
    }

    // ── 더블클릭 판정 (릴리즈 후 400ms 내 2회) ───────────────
    if (!btn.pressed && btn.clickCount > 0) {
        uint32_t sinceRelease = now - btn.releaseMs;
        if (sinceRelease >= BTN_DOUBLE_GAP_MS) {
            // 타임아웃: 누적된 클릭 수로 판정
            if (btn.clickCount >= 2) {
                bool isModeBtn = (btn.pin == PIN_BTN_MODE);
                if (!isModeBtn) _pendingEvent = ButtonEvent::CENTER_DOUBLE;
                // MODE 더블클릭은 현재 미정의
            } else {
                // 싱글 클릭 — short 이벤트 (현재 미사용)
                bool isModeBtn = (btn.pin == PIN_BTN_MODE);
                if (!isModeBtn) _pendingEvent = ButtonEvent::CENTER_SHORT;
                else            _pendingEvent = ButtonEvent::MODE_SHORT;
            }
            btn.clickCount = 0;
        }
    }
}

// ══════════════════════════════════════════════════════════════
//  이벤트 소비 (1회 읽고 NONE으로 클리어)
// ══════════════════════════════════════════════════════════════
ButtonEvent ButtonHandler::getEvent() {
    ButtonEvent ev = _pendingEvent;
    _pendingEvent  = ButtonEvent::NONE;
    return ev;
}
