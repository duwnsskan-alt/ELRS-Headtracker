#pragma once
#include <Arduino.h>
#include "SharedState.h"

// ═══════════════════════════════════════════════════════════════
//  Settings  —  NVS(플래시) 영구 저장
//
//  프로파일 3개 독립 저장. 각 프로파일 키에 인덱스 접미사.
//  예: "p0_maxpan", "p1_maxpan", "p2_maxpan"
// ═══════════════════════════════════════════════════════════════

class Settings {
public:
    // NVS 초기화 + 현재 프로파일 로드 → SharedState 반영
    static void init(SharedState& s);

    // 현재 프로파일 저장 (설정 변경 시 호출)
    static void save(SharedState& s);

    // 프로파일 전환 (0~2), 이전 저장 후 새 프로파일 로드
    static void switchProfile(SharedState& s, uint8_t idx);
    static void nextProfile(SharedState& s);    // 순환

    // 바인드된 Backpack MAC 저장 (별도 키, 프로파일 무관)
    static void saveBackpackMac(const uint8_t mac[6]);
    static bool loadBackpackMac(uint8_t mac[6]);

    // 전체 초기화 (공장 초기화)
    static void factoryReset(SharedState& s);

    static uint8_t currentProfile() { return _currentProfile; }

private:
    static void loadProfile(SharedState& s, uint8_t idx);
    static void saveProfile(const SharedState& s, uint8_t idx);

    static uint8_t _currentProfile;   // 0~2
};
