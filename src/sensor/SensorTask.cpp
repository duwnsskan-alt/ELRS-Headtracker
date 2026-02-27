#include "SensorTask.h"
#include "Orientation.h"
#include "config/Config.h"

#include <Wire.h>
#include <Adafruit_BNO08x.h>

// ── 파일 범위 전역 (이 .cpp에서만) ────────────────────────────
static Adafruit_BNO08x bno(PIN_BNO_RST);
static sh2_SensorValue_t sensorVal;

// ══════════════════════════════════════════════════════════════
//  태스크 시작
// ══════════════════════════════════════════════════════════════
void SensorTask::start() {
    xTaskCreate(
        taskFn,
        "SensorTask",
        TASK_SENSOR_STACK,
        nullptr,
        TASK_SENSOR_PRIO,
        nullptr
    );
}

// ══════════════════════════════════════════════════════════════
//  FreeRTOS 태스크 함수
// ══════════════════════════════════════════════════════════════
void SensorTask::taskFn(void* /*arg*/) {
    Serial.println("[SENSOR] Task started");

    // BNO085 초기화 (실패 시 재시도)
    while (!initSensor()) {
        Serial.println("[SENSOR] Init failed, retry in 2s...");
        {
            if (gState.lock()) {
                gState.sensorOk = false;
                gState.unlock();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    {
        if (gState.lock()) {
            gState.sensorOk = true;
            gState.unlock();
        }
    }
    Serial.println("[SENSOR] BNO085 OK");

    // ── 메인 루프 ──────────────────────────────────────────
    // BNO085는 인터럽트 or polling 모드 선택 가능.
    // 여기서는 polling (PIN_BNO_INT 미연결 기준)
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000 / SENSOR_REPORT_RATE_HZ);

    while (true) {
        readSensor();
        vTaskDelayUntil(&lastWake, period);
    }
}

// ══════════════════════════════════════════════════════════════
//  BNO085 초기화
// ══════════════════════════════════════════════════════════════
bool SensorTask::initSensor() {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);   // 400kHz Fast Mode

    if (!bno.begin_I2C(BNO085_I2C_ADDR, &Wire)) {
        Serial.printf("[SENSOR] BNO085 not found at 0x%02X\n", BNO085_I2C_ADDR);
        return false;
    }

    // Rotation Vector 리포트 활성화
    // BNO085 내부에서 Madgwick/Mahony 퓨전 → 쿼터니언 직접 출력
    // SH2_ROTATION_VECTOR  = 9축 (지자기 포함, 절대 방위)
    // SH2_GAME_ROTATION_VECTOR = 6축 (지자기 없음, 상대 방위, 드리프트 있음)
    //
    // 헤드트래커는 SH2_ROTATION_VECTOR 권장
    // (Pan/Yaw 드리프트 없음, 나침반 기준 절대 방위)
    if (!bno.enableReport(SH2_ROTATION_VECTOR, SENSOR_REPORT_US)) {
        Serial.println("[SENSOR] Failed to enable ROTATION_VECTOR report");
        return false;
    }

    Serial.printf("[SENSOR] BNO085 init OK — report %d Hz\n", SENSOR_REPORT_RATE_HZ);
    return true;
}

// ══════════════════════════════════════════════════════════════
//  한 프레임 읽기
// ══════════════════════════════════════════════════════════════
void SensorTask::readSensor() {
    // BNO085에 새 데이터가 없으면 바로 리턴 (논블로킹)
    if (!bno.getSensorEvent(&sensorVal)) return;

    // 리포트 타입 확인
    if (sensorVal.sensorId != SH2_ROTATION_VECTOR) return;

    // 쿼터니언 추출
    float qw = sensorVal.un.rotationVector.real;
    float qx = sensorVal.un.rotationVector.i;
    float qy = sensorVal.un.rotationVector.j;
    float qz = sensorVal.un.rotationVector.k;

    // 정확도 상태 (0=없음, 1=저, 2=중, 3=고)
    uint8_t accuracy = sensorVal.un.rotationVector.accuracy;
    if (accuracy < 1) {
        // 초기화 직후 보정 미완료 — 데이터는 사용하되 로그만
        static uint32_t lastWarnMs = 0;
        if (millis() - lastWarnMs > 5000) {
            Serial.printf("[SENSOR] Low accuracy: %d (calibrating...)\n", accuracy);
            lastWarnMs = millis();
        }
    }

    // Orientation 클래스로 처리 (오일러 변환 + 채널 매핑)
    Orientation::update(gState, qw, qx, qy, qz);
}
