#include "SBUSOutput.h"
#include <string.h>

// ═══════════════════════════════════════════════════════════════
//  SBUSOutput  —  SBUS 프로토콜 출력 구현
// ═══════════════════════════════════════════════════════════════

bool     SBUSOutput::_initialized = false;
uint32_t SBUSOutput::_sentCount   = 0;
uint32_t SBUSOutput::_failedCount = 0;

// ── UART 초기화 ──────────────────────────────────────────────
bool SBUSOutput::init() {
    if (_initialized) return true;

    // UART 설정: 100000 baud, 8E2, TX 반전
    uart_config_t config = {};
    config.baud_rate  = SBUS_BAUD;          // 100000
    config.data_bits  = UART_DATA_8_BITS;
    config.parity     = UART_PARITY_EVEN;
    config.stop_bits  = UART_STOP_BITS_2;
    config.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t err;

    err = uart_param_config(SBUS_UART, &config);
    if (err != ESP_OK) {
        Serial.printf("[SBUS] uart_param_config failed: %s\n", esp_err_to_name(err));
        return false;
    }

    // TX만 사용 (RX = UART_PIN_NO_CHANGE)
    err = uart_set_pin(SBUS_UART, PIN_SBUS_TX, UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        Serial.printf("[SBUS] uart_set_pin failed: %s\n", esp_err_to_name(err));
        return false;
    }

    // TX 신호 반전 (SBUS 표준: idle=HIGH, data inverted)
    err = uart_set_line_inverse(SBUS_UART, UART_SIGNAL_TXD_INV);
    if (err != ESP_OK) {
        Serial.printf("[SBUS] uart_set_line_inverse failed: %s\n", esp_err_to_name(err));
        return false;
    }

    // UART 드라이버 설치 (TX 버퍼 256, RX 불필요)
    err = uart_driver_install(SBUS_UART, 0, 256, 0, nullptr, 0);
    if (err != ESP_OK) {
        Serial.printf("[SBUS] uart_driver_install failed: %s\n", esp_err_to_name(err));
        return false;
    }

    _initialized = true;
    Serial.printf("[SBUS] Initialized on GPIO%d (UART%d, %dbps, 8E2, inverted)\n",
                  PIN_SBUS_TX, SBUS_UART, SBUS_BAUD);
    return true;
}

// ── CRSF → SBUS 변환 ────────────────────────────────────────
// CRSF: 172~1811  →  SBUS: 0~2047
uint16_t SBUSOutput::crsfToSbus(uint16_t crsf) {
    // 클램프
    if (crsf < CRSF_MIN_US) crsf = CRSF_MIN_US;
    if (crsf > CRSF_MAX_US) crsf = CRSF_MAX_US;

    // 선형 매핑: (crsf - 172) * 2047 / (1811 - 172)
    uint32_t sbus = ((uint32_t)(crsf - CRSF_MIN_US) * 2047) / (CRSF_MAX_US - CRSF_MIN_US);
    return (uint16_t)sbus;
}

// ── SBUS 프레임 빌드 (25바이트) ──────────────────────────────
// 11비트 x 16채널 = 176비트 = 22바이트 (비트패킹)
void SBUSOutput::buildFrame(uint8_t* buf, const uint16_t sbusCh[16]) {
    memset(buf, 0, SBUS_FRAME_LEN);

    buf[0] = SBUS_HEADER;  // 0x0F

    // 11-bit 채널 비트패킹 (LSB first)
    // 채널 0~15 → buf[1]~buf[22]
    buf[1]  =  (uint8_t)(sbusCh[0]        & 0xFF);
    buf[2]  =  (uint8_t)((sbusCh[0] >> 8) | (sbusCh[1] << 3));
    buf[3]  =  (uint8_t)((sbusCh[1] >> 5) | (sbusCh[2] << 6));
    buf[4]  =  (uint8_t)((sbusCh[2] >> 2) & 0xFF);
    buf[5]  =  (uint8_t)((sbusCh[2] >> 10) | (sbusCh[3] << 1));
    buf[6]  =  (uint8_t)((sbusCh[3] >> 7) | (sbusCh[4] << 4));
    buf[7]  =  (uint8_t)((sbusCh[4] >> 4) | (sbusCh[5] << 7));
    buf[8]  =  (uint8_t)((sbusCh[5] >> 1) & 0xFF);
    buf[9]  =  (uint8_t)((sbusCh[5] >> 9) | (sbusCh[6] << 2));
    buf[10] =  (uint8_t)((sbusCh[6] >> 6) | (sbusCh[7] << 5));
    buf[11] =  (uint8_t)((sbusCh[7] >> 3));

    buf[12] =  (uint8_t)(sbusCh[8]        & 0xFF);
    buf[13] =  (uint8_t)((sbusCh[8] >> 8) | (sbusCh[9] << 3));
    buf[14] =  (uint8_t)((sbusCh[9] >> 5) | (sbusCh[10] << 6));
    buf[15] =  (uint8_t)((sbusCh[10] >> 2) & 0xFF);
    buf[16] =  (uint8_t)((sbusCh[10] >> 10) | (sbusCh[11] << 1));
    buf[17] =  (uint8_t)((sbusCh[11] >> 7) | (sbusCh[12] << 4));
    buf[18] =  (uint8_t)((sbusCh[12] >> 4) | (sbusCh[13] << 7));
    buf[19] =  (uint8_t)((sbusCh[13] >> 1) & 0xFF);
    buf[20] =  (uint8_t)((sbusCh[13] >> 9) | (sbusCh[14] << 2));
    buf[21] =  (uint8_t)((sbusCh[14] >> 6) | (sbusCh[15] << 5));
    buf[22] =  (uint8_t)((sbusCh[15] >> 3));

    // FLAGS 바이트: bit0=CH17, bit1=CH18, bit2=frame_lost, bit3=failsafe
    buf[23] = 0x00;  // 모두 정상

    buf[24] = SBUS_FOOTER;  // 0x00
}

// ── 16채널 전체 전송 ─────────────────────────────────────────
void SBUSOutput::send(const uint16_t channels[16]) {
    if (!_initialized) return;

    // CRSF → SBUS 변환
    uint16_t sbusCh[16];
    for (int i = 0; i < 16; i++) {
        sbusCh[i] = crsfToSbus(channels[i]);
    }

    // 프레임 빌드
    uint8_t frame[SBUS_FRAME_LEN];
    buildFrame(frame, sbusCh);

    // UART 전송
    int written = uart_write_bytes(SBUS_UART, frame, SBUS_FRAME_LEN);
    if (written == SBUS_FRAME_LEN) {
        _sentCount++;
    } else {
        _failedCount++;
    }
}

// ── 헤드트래킹 3채널만 전송 ──────────────────────────────────
// HT 채널만 설정, 나머지는 CRSF 중앙값 (992 → SBUS ~1024)
void SBUSOutput::sendHT(uint16_t chPan, uint16_t chTilt, uint16_t chRoll) {
    uint16_t channels[16];
    for (int i = 0; i < 16; i++) {
        channels[i] = CRSF_MID_US;  // 중앙값
    }
    channels[HT_CH_PAN]  = chPan;
    channels[HT_CH_TILT] = chTilt;
    channels[HT_CH_ROLL] = chRoll;

    send(channels);
}
