#pragma once
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
//  Config.h  —  XIAO ESP32-C6 + BNO085 Head Tracker
//  하드웨어 핀 / 상수 / 튜닝 파라미터 전부 여기서 관리
// ═══════════════════════════════════════════════════════════════

// ── 버전 ────────────────────────────────────────────────────────
#define FW_VERSION  "0.1.0-alpha"
#define HW_VERSION  "ESP32C6+BNO085"

// ── 핀 매핑 (XIAO ESP32-C6) ─────────────────────────────────────
// I2C (STEMMA QT / BNO085)
#define PIN_SDA     6    // D4 on XIAO silkscreen
#define PIN_SCL     7    // D5 on XIAO silkscreen
#define PIN_BNO_INT 2    // BNO085 인터럽트 (선택, 미연결 시 polling)
#define PIN_BNO_RST -1   // BNO085 리셋 (미연결 → -1)

// SBUS 출력 (UART1 TX)
// XIAO C6: D6 = GPIO17 → SBUS TX
#define PIN_SBUS_TX 17

// 버튼 (풀업 내장, 눌리면 LOW)
#define PIN_BTN_CENTER  0    // BOOT 버튼 (내장)
#define PIN_BTN_MODE    1    // D1 (추가 버튼)

// RGB LED (WS2812 내장 or 외부)
#define PIN_LED         21   // XIAO C6 내장 RGB LED
// 내장 LED 없으면 외부 WS2812 연결 핀으로 변경

// ── BNO085 설정 ─────────────────────────────────────────────────
#define BNO085_I2C_ADDR     0x4A   // SA0=GND → 0x4A, SA0=VCC → 0x4B
#define SENSOR_REPORT_RATE_HZ   100    // BNO085 출력 주기
#define SENSOR_REPORT_US    (1000000 / SENSOR_REPORT_RATE_HZ)

// ── CRSF / 채널 ─────────────────────────────────────────────────
#define CRSF_MIN_US     172    // CRSF 최솟값
#define CRSF_MID_US     992    // CRSF 중앙값
#define CRSF_MAX_US    1811    // CRSF 최댓값
#define CRSF_RANGE      819    // (CRSF_MAX - CRSF_MID)

// 헤드트래커가 사용할 채널 (0-indexed, ELRS: 0=CH1 ... 15=CH16)
#define HT_CH_PAN       13   // CH14 (ELRS Backpack 헤드트래킹 기본)
#define HT_CH_TILT      14   // CH15
#define HT_CH_ROLL      15   // CH16

// 기본 최대 각도 (도)
#define DEFAULT_MAX_ANGLE_PAN    90.0f
#define DEFAULT_MAX_ANGLE_TILT   60.0f
#define DEFAULT_MAX_ANGLE_ROLL   45.0f

// ── ESP-NOW ──────────────────────────────────────────────────────
#define ESPNOW_CHANNEL      0    // WiFi 채널 (0 = 현재 채널 자동)
#define ESPNOW_SEND_RATE_HZ 50   // 전송 주기
#define ESPNOW_SEND_MS      (1000 / ESPNOW_SEND_RATE_HZ)

// ELRS Backpack 헤드트래킹 패킷 타입
#define ELRS_PKT_TYPE_HT    0x0B

// ── SBUS ────────────────────────────────────────────────────────
#define SBUS_BAUD       100000    // 100kbps 반전 UART
#define SBUS_SEND_RATE_HZ   50
#define SBUS_SEND_MS    (1000 / SBUS_SEND_RATE_HZ)

// ── FreeRTOS 태스크 설정 ─────────────────────────────────────────
#define TASK_SENSOR_STACK   4096
#define TASK_SENSOR_PRIO    5

#define TASK_COMMS_STACK    4096
#define TASK_COMMS_PRIO     4

#define TASK_BUTTON_STACK   2048
#define TASK_BUTTON_PRIO    3

#define TASK_LED_STACK      2048
#define TASK_LED_PRIO       1

// ── 버튼 타이밍 (ms) ────────────────────────────────────────────
#define BTN_DEBOUNCE_MS     30
#define BTN_SHORT_MS        50    // 이상이면 short press로 인식
#define BTN_LONG_MS         1000  // 이상 홀드 = long press
#define BTN_VERY_LONG_MS    3000  // 이상 홀드 = very long (WiFi AP)
#define BTN_DOUBLE_GAP_MS   400   // 더블클릭 인터벌

// ── NVS 키 ──────────────────────────────────────────────────────
#define NVS_NAMESPACE       "htconfig"
#define NVS_KEY_PROFILE     "profile"    // 현재 프로파일 번호
#define NVS_KEY_COMMS_MODE  "commsmode"  // 0=ESPNOW, 1=SBUS
#define NVS_KEY_ESPNOW_MAC  "enmac"      // 6바이트 Backpack MAC
#define NVS_KEY_MAX_PAN     "maxpan"
#define NVS_KEY_MAX_TILT    "maxtilt"
#define NVS_KEY_MAX_ROLL    "maxroll"
#define NVS_KEY_INV_PAN     "invpan"
#define NVS_KEY_INV_TILT    "invtilt"
#define NVS_KEY_INV_ROLL    "invroll"

// ── 통신 모드 ────────────────────────────────────────────────────
enum CommsMode : uint8_t {
    COMMS_ESPNOW = 0,
    COMMS_SBUS   = 1,
};
