#pragma once
#include <Arduino.h>
#include "SharedState.h"

// ═══════════════════════════════════════════════════════════════
//  ESPNowComms  —  ELRS Backpack으로 헤드트래킹 데이터 전송
//
//  ELRS Backpack ESP-NOW 통신 방식:
//  1. Backpack MAC 주소로 직접 전송 (유니캐스트)
//     → 바인드 절차 필요 (Backpack에서 먼저 scan)
//  2. 또는 브로드캐스트 (FF:FF:FF:FF:FF:FF)
//     → 바인드 불필요, 범위 내 모든 Backpack 수신
//     → 단점: 보안 없음, 혼선 가능
//
//  초기 구현: 브로드캐스트로 시작, 이후 유니캐스트 바인드 추가
// ═══════════════════════════════════════════════════════════════

class ESPNowComms {
public:
    // 초기화 (WiFi 포함)
    static bool init();

    // 채널 데이터 전송 (50Hz 호출 예정)
    static void send(uint16_t chPan, uint16_t chTilt, uint16_t chRoll);

    // 피어 MAC 설정 (브로드캐스트 → 특정 Backpack)
    static void setPeerMac(const uint8_t mac[6]);

    // 전송 통계
    static uint32_t getSentCount()   { return _sentCount;   }
    static uint32_t getFailedCount() { return _failedCount; }

private:
    static void onSendCallback(const uint8_t* mac, esp_now_send_status_t status);

    static uint8_t  _peerMac[6];
    static uint32_t _sentCount;
    static uint32_t _failedCount;
    static bool     _initialized;
};
