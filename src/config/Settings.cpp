#include "Settings.h"
#include "config/Config.h"
#include <Preferences.h>

// ── 정적 멤버 ─────────────────────────────────────────────────
uint8_t Settings::_currentProfile = 0;

static Preferences prefs;

// NVS 키 헬퍼 (프로파일 인덱스 접미사)
static String pk(const char* key, uint8_t idx) {
    return String("p") + idx + "_" + key;
}

// ══════════════════════════════════════════════════════════════
void Settings::init(SharedState& s) {
    prefs.begin(NVS_NAMESPACE, false);

    _currentProfile = prefs.getUChar(NVS_KEY_PROFILE, 0);
    if (_currentProfile > 2) _currentProfile = 0;

    loadProfile(s, _currentProfile);

    // 바인드된 MAC 로드
    uint8_t mac[6];
    if (loadBackpackMac(mac)) {
        if (s.lock()) {
            memcpy(s.config.espnowMac, mac, 6);
            s.unlock();
        }
    }

    prefs.end();
    Serial.printf("[NVS] Loaded profile %d\n", _currentProfile);
}

// ══════════════════════════════════════════════════════════════
void Settings::save(SharedState& s) {
    prefs.begin(NVS_NAMESPACE, false);
    saveProfile(s, _currentProfile);
    prefs.end();
    Serial.printf("[NVS] Saved profile %d\n", _currentProfile);
}

// ══════════════════════════════════════════════════════════════
void Settings::switchProfile(SharedState& s, uint8_t idx) {
    if (idx > 2) return;
    // 현재 프로파일 저장
    prefs.begin(NVS_NAMESPACE, false);
    saveProfile(s, _currentProfile);
    // 새 프로파일 로드
    _currentProfile = idx;
    prefs.putUChar(NVS_KEY_PROFILE, _currentProfile);
    loadProfile(s, _currentProfile);
    prefs.end();
    Serial.printf("[NVS] Switched to profile %d\n", _currentProfile);
}

void Settings::nextProfile(SharedState& s) {
    switchProfile(s, (_currentProfile + 1) % 3);
}

// ══════════════════════════════════════════════════════════════
void Settings::saveBackpackMac(const uint8_t mac[6]) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBytes("bp_mac", mac, 6);
    prefs.end();
    Serial.printf("[NVS] Backpack MAC saved: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

bool Settings::loadBackpackMac(uint8_t mac[6]) {
    size_t len = prefs.getBytes("bp_mac", mac, 6);
    if (len != 6) {
        memset(mac, 0xFF, 6);   // 브로드캐스트 기본값
        return false;
    }
    // 모두 FF면 미바인드 상태
    bool isBcast = true;
    for (int i = 0; i < 6; i++) if (mac[i] != 0xFF) { isBcast = false; break; }
    return !isBcast;
}

// ══════════════════════════════════════════════════════════════
void Settings::factoryReset(SharedState& s) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();

    // SharedState 기본값으로 복원
    if (s.lock()) {
        s.config.maxPan    = DEFAULT_MAX_ANGLE_PAN;
        s.config.maxTilt   = DEFAULT_MAX_ANGLE_TILT;
        s.config.maxRoll   = DEFAULT_MAX_ANGLE_ROLL;
        s.config.invPan    = false;
        s.config.invTilt   = false;
        s.config.invRoll   = false;
        s.config.commsMode = COMMS_ESPNOW;
        memset(s.config.espnowMac, 0xFF, 6);
        s.unlock();
    }
    _currentProfile = 0;
    Serial.println("[NVS] Factory reset done");
}

// ══════════════════════════════════════════════════════════════
//  private helpers
// ══════════════════════════════════════════════════════════════
void Settings::loadProfile(SharedState& s, uint8_t idx) {
    if (!s.lock()) return;
    s.config.maxPan    = prefs.getFloat(pk("maxpan",  idx).c_str(), DEFAULT_MAX_ANGLE_PAN);
    s.config.maxTilt   = prefs.getFloat(pk("maxtilt", idx).c_str(), DEFAULT_MAX_ANGLE_TILT);
    s.config.maxRoll   = prefs.getFloat(pk("maxroll", idx).c_str(), DEFAULT_MAX_ANGLE_ROLL);
    s.config.invPan    = prefs.getBool( pk("invpan",  idx).c_str(), false);
    s.config.invTilt   = prefs.getBool( pk("invtilt", idx).c_str(), false);
    s.config.invRoll   = prefs.getBool( pk("invroll", idx).c_str(), false);
    s.config.commsMode = (CommsMode)prefs.getUChar(pk("comms", idx).c_str(), COMMS_ESPNOW);
    s.unlock();
}

void Settings::saveProfile(const SharedState& s, uint8_t idx) {
    // const_cast: save는 읽기 전용이지만 SharedState.lock()이 non-const
    SharedState& ns = const_cast<SharedState&>(s);
    if (!ns.lock()) return;
    prefs.putFloat(pk("maxpan",  idx).c_str(), s.config.maxPan);
    prefs.putFloat(pk("maxtilt", idx).c_str(), s.config.maxTilt);
    prefs.putFloat(pk("maxroll", idx).c_str(), s.config.maxRoll);
    prefs.putBool( pk("invpan",  idx).c_str(), s.config.invPan);
    prefs.putBool( pk("invtilt", idx).c_str(), s.config.invTilt);
    prefs.putBool( pk("invroll", idx).c_str(), s.config.invRoll);
    prefs.putUChar(pk("comms",   idx).c_str(), (uint8_t)s.config.commsMode);
    ns.unlock();
}
