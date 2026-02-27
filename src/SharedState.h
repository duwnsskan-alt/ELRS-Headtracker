#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config/Config.h"

// ═══════════════════════════════════════════════════════════════
//  SharedState  —  모든 태스크가 읽고 쓰는 전역 상태
//  접근은 반드시 mutex를 통해서만 (lock/unlock 헬퍼 사용)
// ═══════════════════════════════════════════════════════════════

struct OrientationData {
    // BNO085 원시 쿼터니언
    float qw = 1.0f, qx = 0.0f, qy = 0.0f, qz = 0.0f;

    // 센터 기준 오일러각 (도) — 센터링 후 적용
    float pan  = 0.0f;   // Yaw   (좌우, -180~+180)
    float tilt = 0.0f;   // Pitch (상하, -90~+90)
    float roll = 0.0f;   // Roll  (기울기, -180~+180)

    uint64_t timestamp_us = 0;   // 마지막 업데이트 시각
};

struct ChannelData {
    // CRSF 범위 채널값 (172~1811, 중앙 992)
    uint16_t ch[16] = {};   // ch[0]=CH1 ... ch[15]=CH16

    // 헤드트래킹 채널 바로가기
    uint16_t& pan()  { return ch[HT_CH_PAN];  }
    uint16_t& tilt() { return ch[HT_CH_TILT]; }
    uint16_t& roll() { return ch[HT_CH_ROLL]; }
};

// ── 프로파일 설정 (NVS에서 로드/저장) ──────────────────────────
struct ProfileConfig {
    float    maxPan  = DEFAULT_MAX_ANGLE_PAN;
    float    maxTilt = DEFAULT_MAX_ANGLE_TILT;
    float    maxRoll = DEFAULT_MAX_ANGLE_ROLL;
    bool     invPan  = false;
    bool     invTilt = false;
    bool     invRoll = false;
    CommsMode commsMode = COMMS_ESPNOW;
    uint8_t  espnowMac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; // 브로드캐스트
};

// ── 전체 공유 상태 ──────────────────────────────────────────────
struct SharedState {
    // 데이터
    OrientationData orientation;
    ChannelData     channels;
    ProfileConfig   config;

    // 상태 플래그
    bool sensorOk        = false;
    bool tracking        = true;   // false = 출력 중지
    bool espnowConnected = false;
    bool wifiApActive    = false;

    // 센터 저장 쿼터니언 (센터 리셋 시 업데이트)
    float centerQw = 1.0f, centerQx = 0.0f,
          centerQy = 0.0f, centerQz = 0.0f;

    // 동기화 핸들
    SemaphoreHandle_t mutex = nullptr;

    // ── 초기화 ────────────────────────────────────────────────
    void init() {
        mutex = xSemaphoreCreateMutex();
        configASSERT(mutex != nullptr);
    }

    // ── 락 헬퍼 (RAII 스타일) ────────────────────────────────
    bool lock(TickType_t timeout = pdMS_TO_TICKS(10)) {
        return xSemaphoreTake(mutex, timeout) == pdTRUE;
    }
    void unlock() {
        xSemaphoreGive(mutex);
    }
};

// 전역 싱글톤 — main.cpp에서 정의, 나머지는 extern으로 참조
extern SharedState gState;
