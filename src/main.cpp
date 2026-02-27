#include <Arduino.h>
#include "SharedState.h"
#include "sensor/SensorTask.h"
#include "comms/CommsTask.h"
#include "button/ButtonTask.h"
#include "sensor/Orientation.h"
#include "config/Config.h"

// ═══════════════════════════════════════════════════════════════
//  main.cpp  —  진입점
//
//  Phase 1 (현재): Sensor + ESP-NOW 핵심 동작 검증
//  Phase 2 (추후): 버튼, LED, WebUI, SBUS, NVS 설정 추가
// ═══════════════════════════════════════════════════════════════

// ── 전역 SharedState 정의 (extern은 SharedState.h에서 선언) ──
SharedState gState;

// ══════════════════════════════════════════════════════════════
//  setup()
// ══════════════════════════════════════════════════════════════
void setup() {
    // ── 시리얼 ────────────────────────────────────────────────
    Serial.begin(115200);
    delay(500);   // USB CDC 안정화 대기

    Serial.println("\n\n");
    Serial.println("╔═══════════════════════════════╗");
    Serial.printf ("║  HeadTracker  FW %s  ║\n", FW_VERSION);
    Serial.printf ("║  HW: %-25s║\n", HW_VERSION);
    Serial.println("╚═══════════════════════════════╝");

    // ── SharedState 초기화 ────────────────────────────────────
    gState.init();

    // 기본 설정 적용 (Phase 2에서 NVS 로드로 교체)
    gState.config.maxPan    = DEFAULT_MAX_ANGLE_PAN;
    gState.config.maxTilt   = DEFAULT_MAX_ANGLE_TILT;
    gState.config.maxRoll   = DEFAULT_MAX_ANGLE_ROLL;
    gState.config.commsMode = COMMS_ESPNOW;
    // 브로드캐스트 MAC (기본값, 바인드 전)
    memset(gState.config.espnowMac, 0xFF, 6);

    Serial.println("[MAIN] SharedState OK");

    // ── FreeRTOS 태스크 생성 ─────────────────────────────────
    SensorTask::start();
    CommsTask::start();
    ButtonTask::start();

    Serial.println("[MAIN] Tasks started");
    Serial.println("[MAIN] Boot complete — monitoring...\n");
}

// ══════════════════════════════════════════════════════════════
//  loop()  —  메인 루프는 최소화 (태스크에 위임)
// ══════════════════════════════════════════════════════════════
void loop() {
    // Phase 1: 시리얼 명령어 (디버깅 편의용)
    // 'c' → 센터 리셋
    // 't' → 트래킹 토글
    // 's' → 상태 덤프
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
        case 'c':
            Orientation::resetCenter(gState);
            Serial.println("[CMD] Center reset");
            break;

        case 't':
            if (gState.lock()) {
                gState.tracking = !gState.tracking;
                bool t = gState.tracking;
                gState.unlock();
                Serial.printf("[CMD] Tracking: %s\n", t ? "ON" : "OFF");
            }
            break;

        case 's': {
            if (!gState.lock()) break;
            Serial.println("\n── State Dump ──────────────────");
            Serial.printf("  SensorOK   : %s\n", gState.sensorOk ? "YES" : "NO");
            Serial.printf("  Tracking   : %s\n", gState.tracking  ? "ON"  : "OFF");
            Serial.printf("  ESP-NOW    : %s\n", gState.espnowConnected ? "CONN" : "DISC");
            Serial.printf("  Pan/Tilt/Roll: %.1f° / %.1f° / %.1f°\n",
                          gState.orientation.pan,
                          gState.orientation.tilt,
                          gState.orientation.roll);
            Serial.printf("  CH14/15/16 : %d / %d / %d\n",
                          gState.channels.pan(),
                          gState.channels.tilt(),
                          gState.channels.roll());
            Serial.println("────────────────────────────────\n");
            gState.unlock();
            break;
        }

        case 'h':
            Serial.println("[CMD] Commands: c=center  t=toggle  s=status  h=help");
            break;

        default:
            break;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(50));   // loop()는 50ms 간격으로 실행
}
