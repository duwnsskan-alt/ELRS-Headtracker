#include "ButtonTask.h"
#include "ButtonHandler.h"
#include "LEDController.h"
#include "WebUI.h"
#include "config/Settings.h"
#include "sensor/Orientation.h"
#include "SharedState.h"
#include "config/Config.h"

void ButtonTask::start() {
    xTaskCreate(taskFn, "ButtonTask", TASK_BUTTON_STACK, nullptr, TASK_BUTTON_PRIO, nullptr);
}

void ButtonTask::taskFn(void*) {
    ButtonHandler::init();
    Serial.println("[BTN] Task started");

    TickType_t lastWake = xTaskGetTickCount();

    while (true) {
        ButtonHandler::update();
        ButtonEvent ev = ButtonHandler::getEvent();

        switch (ev) {

        // ── CENTER 더블클릭: 트래킹 ON/OFF ───────────────────
        case ButtonEvent::CENTER_DOUBLE:
            if (gState.lock()) {
                gState.tracking = !gState.tracking;
                bool on = gState.tracking;
                gState.unlock();
                Serial.printf("[BTN] Tracking: %s\n", on ? "ON" : "OFF");
                if (on) LEDController::setState(LEDState::ESPNOW_SEARCHING);
                else    LEDController::setState(LEDState::OFF);
            }
            break;

        // ── CENTER 1초: 센터 리셋 ─────────────────────────────
        case ButtonEvent::CENTER_LONG:
            Orientation::resetCenter(gState);
            LEDController::flashWhite();
            Serial.println("[BTN] Center reset");
            break;

        // ── MODE 1초: 프로파일 순환 ───────────────────────────
        case ButtonEvent::MODE_LONG:
            Settings::nextProfile(gState);
            {
                uint8_t p = Settings::currentProfile();
                Serial.printf("[BTN] Profile → %d\n", p + 1);
                // 프로파일 번호만큼 LED 깜빡임 (1~3회)
                for (uint8_t i = 0; i <= p; i++) {
                    LEDController::flashWhite();
                    delay(150);
                }
            }
            break;

        // ── MODE 2초: 통신 모드 전환 ──────────────────────────
        case ButtonEvent::MODE_VERY_LONG:
            if (gState.lock()) {
                CommsMode next = (gState.config.commsMode == COMMS_ESPNOW)
                                 ? COMMS_SBUS : COMMS_ESPNOW;
                gState.config.commsMode = next;
                gState.unlock();
                Settings::save(gState);
                Serial.printf("[BTN] CommsMode → %s\n",
                              next == COMMS_ESPNOW ? "ESP-NOW" : "SBUS");
                LEDController::setState(next == COMMS_ESPNOW
                                        ? LEDState::ESPNOW_SEARCHING
                                        : LEDState::SBUS_ACTIVE);
            }
            break;

        // ── CENTER+MODE 3초: WiFi AP 토글 ─────────────────────
        case ButtonEvent::BOTH_VERY_LONG:
            if (WebUI::isRunning()) {
                WebUI::stop();
                Serial.println("[BTN] WiFi AP: OFF");
            } else {
                WebUI::start();
                Serial.println("[BTN] WiFi AP: ON → connect to 'HeadTracker' → 192.168.4.1");
            }
            break;

        default:
            break;
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(20));
    }
}
