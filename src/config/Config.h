#pragma once
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
//  Config.h  —  XIAO ESP32-C6 + BNO085 Head Tracker
//  하드웨어 핀 / 상수 / 튜닝 파라미터 전부 여기서 관리
//
//  ★ 미결 사항 (HW 도착 후 확정):
//    - PIN_BTN_MODE : MODE 버튼 핀 번호
// ═══════════════════════════════════════════════════════════════

// ── 버전 ────────────────────────────────────────────────────────
#define FW_VERSION  "0.2.0-alpha"
#define HW_VERSION  "ESP32C6+BNO085"

// ── 핀 매핑 ──────────────────────────────────────────────────────
// 보드 선택: platformio.ini build_flags -DBOARD_ESP32C3_SUPERMINI

#if defined(BOARD_ESP32C3_SUPERMINI)
// ┌─────────────────────────────────────────────────────────────┐
// │  ESP32-C3 SuperMini  (테스트 보드)                           │
// │  BOOT 버튼: GPIO9  /  MODE 버튼: GPIO10                     │
// │  내장 WS2812 없음 → GPIO3에 외부 NeoPixel 연결              │
// └─────────────────────────────────────────────────────────────┘
#define PIN_SDA         6    // I2C SDA (BNO085)
#define PIN_SCL         7    // I2C SCL (BNO085)
#define PIN_BNO_INT    -1    // 미연결 → polling 모드
#define PIN_BNO_RST    -1    // 미연결

#define PIN_SBUS_TX     4    // UART1 TX

#define PIN_BTN_CENTER  9    // BOOT 버튼 (내장, 풀업, LOW=눌림)
#define PIN_BTN_MODE   10    // MODE 버튼 (GPIO10, 확정)

// 외부 WS2812 NeoPixel (GPIO3 권장 — 스트래핑 핀 아님)
#define PIN_LED         3
#define PIN_LED_BUILTIN 8    // 내장 파란 LED (active LOW, 상태 확인용)

#else
// ┌─────────────────────────────────────────────────────────────┐
// │  Seeed XIAO ESP32-C6  (양산 보드)                           │
// └─────────────────────────────────────────────────────────────┘
#define PIN_SDA     6    // D4 on XIAO silkscreen
#define PIN_SCL     7    // D5 on XIAO silkscreen
#define PIN_BNO_INT 2    // BNO085 인터럽트 (선택, 미연결 시 polling)
#define PIN_BNO_RST -1   // BNO085 리셋 (미연결 → -1)

#define PIN_SBUS_TX 17   // D6 = GPIO17

#define PIN_BTN_CENTER  0    // BOOT 버튼 (내장, 확정)
// ⚠️ TODO: HW 도착 후 실제 핀 번호로 변경
#define PIN_BTN_MODE    1    // ← 미결: HW 확인 필요

// RGB LED — XIAO ESP32-C6 내장 WS2812 (GPIO21)
#define PIN_LED         21
#define LED_BUILTIN_RGB         // 내장 LED 사용 중임을 명시

#endif

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
#define ESPNOW_CHANNEL      6    // ELRS Backpack 기본 채널 (6 고정)
#define ESPNOW_SEND_RATE_HZ 50   // 전송 주기
#define ESPNOW_SEND_MS      (1000 / ESPNOW_SEND_RATE_HZ)
// 바인드 방식: WebUI에서 주변 Backpack MAC 스캔 후 선택 (버튼 없음)
// 바인드 전: 브로드캐스트 (FF:FF:FF:FF:FF:FF)
// 바인드 후: NVS에 저장된 MAC으로 유니캐스트

// ELRS Backpack 헤드트래킹 패킷 타입
#define ELRS_PKT_TYPE_HT    0x0B

// ── BLE 트레이너 (FrSky PARA) ────────────────────────────────────
// ⏳ 추후 구현 예정 (Phase 5)
// ESP32-C6 BLE 5.3으로 PARA 프로토콜 구현 가능
// nRF52840 기반 오픈소스(ysoldak/HeadTracker) 포팅 필요
// #define ENABLE_BLE_PARA   // 활성화 시 추가 개발 필요

// ── WebUI ────────────────────────────────────────────────────────
#define WIFI_AP_SSID        "HeadTracker"   // AP 이름
#define WIFI_AP_PASSWORD    ""              // 비번 없음 (편의성)
#define WIFI_AP_CHANNEL     1
#define WEBUI_PORT          80
#define WEBUI_HOSTNAME      "headtracker"   // headtracker.local
// WebUI 기능:
//   - 채널 매핑 / 각도 범위 / 반전 설정 (ELRS Configurator 스타일)
//   - 실시간 3D 헤드 시각화 (디버그 탭)
//   - Backpack 스캔 & 바인드 (ESP-NOW MAC 선택)
//   - 프로파일 3개 전환
//   - OTA 펌웨어 업데이트

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
