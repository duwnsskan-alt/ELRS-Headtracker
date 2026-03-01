#pragma once
#include <Arduino.h>
#include <driver/uart.h>
#include "config/Config.h"

// ═══════════════════════════════════════════════════════════════
//  SBUSOutput  —  SBUS 프로토콜 출력 (UART1, 100kbps, 반전)
//
//  SBUS 프레임 (25바이트):
//  [0x0F][CH1..CH16 packed 11-bit x 16][FLAGS][0x00]
//
//  - 100000 baud, 8E2 (8 data, even parity, 2 stop)
//  - 신호 반전 (SBUS 표준, ESP32 UART 하드웨어 지원)
//  - 채널 범위: 0~2047 (CRSF 172~1811 → SBUS 0~2047 매핑)
//  - 50Hz 전송 (20ms 주기)
// ═══════════════════════════════════════════════════════════════

class SBUSOutput {
public:
    // UART 초기화 (GPIO17, UART1, 100kbps 반전)
    // 반환: true = 성공
    static bool init();

    // 전체 16채널 SBUS 프레임 전송
    // channels[]: CRSF 범위 값 (172~1811), 16개
    static void send(const uint16_t channels[16]);

    // 헤드트래킹 3채널만 전송 (나머지 중앙값)
    static void sendHT(uint16_t chPan, uint16_t chTilt, uint16_t chRoll);

    // 전송 통계
    static uint32_t getSentCount()   { return _sentCount; }
    static uint32_t getFailedCount() { return _failedCount; }

    // CRSF → SBUS 채널 변환
    // CRSF: 172~1811 → SBUS: 0~2047
    static uint16_t crsfToSbus(uint16_t crsf);

    static bool isInitialized() { return _initialized; }

private:
    // 25바이트 SBUS 프레임 빌드
    static void buildFrame(uint8_t* buf, const uint16_t sbusCh[16]);

    static bool     _initialized;
    static uint32_t _sentCount;
    static uint32_t _failedCount;

    static constexpr uint8_t  SBUS_HEADER    = 0x0F;
    static constexpr uint8_t  SBUS_FOOTER    = 0x00;
    static constexpr uint8_t  SBUS_FRAME_LEN = 25;
    static constexpr uart_port_t SBUS_UART   = UART_NUM_1;
};
