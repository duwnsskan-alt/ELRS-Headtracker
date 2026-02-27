#pragma once
#include <Arduino.h>
#include "config/Config.h"

// ═══════════════════════════════════════════════════════════════
//  CRSF.h  —  ELRS Backpack 헤드트래킹 패킷 빌더
//
//  ELRS Backpack이 기대하는 패킷 형식:
//  [SYNC][LEN][TYPE][PAYLOAD...][CRC8]
//
//  HeadTracking 타입(0x0B) payload:
//  ch1_lo, ch1_hi, ch2_lo, ch2_hi, ch3_lo, ch3_hi
//  (각 채널 2바이트 little-endian, 172~1811)
// ═══════════════════════════════════════════════════════════════

class CRSF {
public:
    // ELRS Backpack HeadTracking 패킷 크기
    // SYNC(1) + LEN(1) + TYPE(1) + ch1(2) + ch2(2) + ch3(2) + CRC(1) = 11바이트
    static constexpr uint8_t HT_PACKET_SIZE = 11;

    // ── HeadTracking 패킷 빌드 ────────────────────────────────
    // buf: 최소 HT_PACKET_SIZE 바이트 버퍼
    // ch_pan/tilt/roll: CRSF 범위 (172~1811)
    // 반환: 실제 기록된 바이트 수
    static uint8_t buildHeadTrackPacket(uint8_t* buf,
                                         uint16_t ch_pan,
                                         uint16_t ch_tilt,
                                         uint16_t ch_roll) {
        uint8_t i = 0;
        buf[i++] = 0xC8;              // CRSF SYNC (Broadcast)
        buf[i++] = 8;                 // LEN = TYPE + PAYLOAD + CRC
        buf[i++] = ELRS_PKT_TYPE_HT; // 0x0B

        // Payload: 채널 3개 (little-endian 2바이트)
        buf[i++] = (uint8_t)(ch_pan  & 0xFF);
        buf[i++] = (uint8_t)(ch_pan  >> 8);
        buf[i++] = (uint8_t)(ch_tilt & 0xFF);
        buf[i++] = (uint8_t)(ch_tilt >> 8);
        buf[i++] = (uint8_t)(ch_roll & 0xFF);
        buf[i++] = (uint8_t)(ch_roll >> 8);

        // CRC8 (DVB-S2, TYPE~PAYLOAD 범위)
        buf[i++] = crc8(buf + 2, i - 2);

        return i;   // = HT_PACKET_SIZE
    }

private:
    // ── CRC-8 DVB-S2 ─────────────────────────────────────────
    // ELRS/CRSF 표준 CRC 알고리즘
    static uint8_t crc8(const uint8_t* data, uint8_t len) {
        uint8_t crc = 0;
        for (uint8_t i = 0; i < len; i++) {
            crc ^= data[i];
            for (uint8_t b = 0; b < 8; b++) {
                crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : (crc << 1);
            }
        }
        return crc;
    }
};
