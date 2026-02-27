#include "ESPNowComms.h"
#include "crsf/CRSF.h"
#include "config/Config.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ── 정적 멤버 초기화 ──────────────────────────────────────────
uint8_t  ESPNowComms::_peerMac[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint32_t ESPNowComms::_sentCount   = 0;
uint32_t ESPNowComms::_failedCount = 0;
bool     ESPNowComms::_initialized = false;

// ══════════════════════════════════════════════════════════════
//  초기화
// ══════════════════════════════════════════════════════════════
bool ESPNowComms::init() {
    if (_initialized) return true;

    // ── 1. WiFi Station 모드 (ESP-NOW 선행 조건) ────────────
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // ESP32-C6: WiFi 채널을 ELRS Backpack과 맞춰야 함
    // ELRS는 기본적으로 채널 6 사용 (Backpack도 동일)
    // esp_wifi_set_channel로 강제 설정
    esp_wifi_set_channel(ESPNOW_CHANNEL == 0 ? 6 : ESPNOW_CHANNEL,
                         WIFI_SECOND_CHAN_NONE);

    // ── 2. ESP-NOW 초기화 ────────────────────────────────────
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESPNOW] esp_now_init() failed");
        return false;
    }

    // ── 3. 전송 콜백 등록 ────────────────────────────────────
    esp_now_register_send_cb(onSendCallback);

    // ── 4. 피어 등록 (브로드캐스트) ─────────────────────────
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, _peerMac, 6);
    peer.channel   = ESPNOW_CHANNEL == 0 ? 6 : ESPNOW_CHANNEL;
    peer.encrypt   = false;
    peer.ifidx     = WIFI_IF_STA;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("[ESPNOW] Failed to add peer");
        esp_now_deinit();
        return false;
    }

    _initialized = true;

    // 내 MAC 출력 (Backpack 바인드 참조용)
    Serial.printf("[ESPNOW] Init OK — My MAC: %s\n",
                  WiFi.macAddress().c_str());
    Serial.printf("[ESPNOW] Peer MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  _peerMac[0], _peerMac[1], _peerMac[2],
                  _peerMac[3], _peerMac[4], _peerMac[5]);
    return true;
}

// ══════════════════════════════════════════════════════════════
//  패킷 전송
// ══════════════════════════════════════════════════════════════
void ESPNowComms::send(uint16_t chPan, uint16_t chTilt, uint16_t chRoll) {
    if (!_initialized) return;

    uint8_t buf[CRSF::HT_PACKET_SIZE];
    uint8_t len = CRSF::buildHeadTrackPacket(buf, chPan, chTilt, chRoll);

    esp_err_t result = esp_now_send(_peerMac, buf, len);

    if (result != ESP_OK) {
        _failedCount++;
        if (_failedCount % 100 == 0) {
            Serial.printf("[ESPNOW] Send error 0x%X (fail#%lu)\n",
                          result, _failedCount);
        }
    }
}

// ══════════════════════════════════════════════════════════════
//  피어 MAC 변경 (바인드 후 유니캐스트 전환)
// ══════════════════════════════════════════════════════════════
void ESPNowComms::setPeerMac(const uint8_t mac[6]) {
    // 기존 피어 제거
    if (_initialized) {
        esp_now_del_peer(_peerMac);
    }

    memcpy(_peerMac, mac, 6);

    if (_initialized) {
        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, _peerMac, 6);
        peer.channel = ESPNOW_CHANNEL == 0 ? 6 : ESPNOW_CHANNEL;
        peer.encrypt = false;
        peer.ifidx   = WIFI_IF_STA;
        esp_now_add_peer(&peer);
        Serial.printf("[ESPNOW] Peer updated: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

// ══════════════════════════════════════════════════════════════
//  전송 완료 콜백
// ══════════════════════════════════════════════════════════════
void ESPNowComms::onSendCallback(const uint8_t* /*mac*/,
                                  esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        _sentCount++;
    } else {
        _failedCount++;
    }

    // gState 업데이트 (Backpack 수신 확인은 별도 로직 필요)
    // 현재는 전송 성공 여부로만 판단
    if (gState.lock()) {
        gState.espnowConnected = (status == ESP_NOW_SEND_SUCCESS);
        gState.unlock();
    }
}
