#include "CommsTask.h"
#include "ESPNowComms.h"
#include "config/Config.h"

// ══════════════════════════════════════════════════════════════
//  태스크 시작
// ══════════════════════════════════════════════════════════════
void CommsTask::start() {
    xTaskCreate(
        taskFn,
        "CommsTask",
        TASK_COMMS_STACK,
        nullptr,
        TASK_COMMS_PRIO,
        nullptr
    );
}

// ══════════════════════════════════════════════════════════════
//  FreeRTOS 태스크 함수
// ══════════════════════════════════════════════════════════════
void CommsTask::taskFn(void* /*arg*/) {
    Serial.println("[COMMS] Task started");

    // ── ESP-NOW 초기화 ────────────────────────────────────────
    CommsMode mode = COMMS_ESPNOW;
    {
        if (gState.lock()) {
            mode = gState.config.commsMode;
            gState.unlock();
        }
    }

    if (mode == COMMS_ESPNOW) {
        if (!ESPNowComms::init()) {
            Serial.println("[COMMS] ESP-NOW init failed, falling back to SBUS");
            if (gState.lock()) {
                gState.config.commsMode = COMMS_SBUS;
                gState.unlock();
            }
        } else {
            Serial.println("[COMMS] ESP-NOW ready");
        }
    }

    // ── 메인 루프 (50Hz) ─────────────────────────────────────
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(ESPNOW_SEND_MS);

    uint32_t loopCount = 0;

    while (true) {
        // 현재 채널값 스냅샷
        uint16_t chPan, chTilt, chRoll;
        bool tracking;
        bool sensorOk;
        CommsMode currentMode;

        if (gState.lock()) {
            chPan       = gState.channels.pan();
            chTilt      = gState.channels.tilt();
            chRoll      = gState.channels.roll();
            tracking    = gState.tracking;
            sensorOk    = gState.sensorOk;
            currentMode = gState.config.commsMode;
            gState.unlock();
        } else {
            vTaskDelayUntil(&lastWake, period);
            continue;
        }

        // 트래킹 OFF나 센서 오류면 중앙값 전송
        if (!tracking || !sensorOk) {
            chPan = chTilt = chRoll = CRSF_MID_US;
        }

        // ── 전송 ────────────────────────────────────────────
        if (currentMode == COMMS_ESPNOW) {
            ESPNowComms::send(chPan, chTilt, chRoll);
        }
        // SBUS는 SBUSOutput 태스크에서 별도 처리 (추후 추가)

        // ── 1초마다 진단 출력 ────────────────────────────────
        loopCount++;
        if (loopCount % 50 == 0) {   // 50Hz * 50 = 1초
            float pan, tilt, roll;
            if (gState.lock()) {
                pan  = gState.orientation.pan;
                tilt = gState.orientation.tilt;
                roll = gState.orientation.roll;
                gState.unlock();
            }
            Serial.printf("[COMMS] Pan:%.1f° Tilt:%.1f° Roll:%.1f° "
                          "| CH: %d/%d/%d | TX:%lu Fail:%lu\n",
                          pan, tilt, roll,
                          chPan, chTilt, chRoll,
                          ESPNowComms::getSentCount(),
                          ESPNowComms::getFailedCount());
        }

        vTaskDelayUntil(&lastWake, period);
    }
}
