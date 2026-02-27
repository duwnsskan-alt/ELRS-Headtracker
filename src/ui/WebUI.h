#pragma once
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
//  WebUI  —  WiFi AP + AsyncWebServer
//
//  접속: WiFi "HeadTracker" → http://headtracker.local 또는 192.168.4.1
//
//  페이지 구성:
//  ┌─────────────────────────────────────────────────────────┐
//  │  [Settings]  [Bind]  [Debug 3D]           ● Connected  │
//  ├─────────────────────────────────────────────────────────┤
//  │ Settings 탭:                                            │
//  │   Channel Assignment (Pan/Tilt/Roll → CH14~CH18)       │
//  │   Max Angle, Invert, Profile 선택                       │
//  │   Comms Mode (ESP-NOW / SBUS)                           │
//  │   OTA Firmware Update                                   │
//  ├─────────────────────────────────────────────────────────┤
//  │ Bind 탭:                                                │
//  │   주변 ELRS Backpack 스캔 → MAC 목록 → 선택 → 바인드    │
//  │   현재 바인드 상태 표시                                  │
//  ├─────────────────────────────────────────────────────────┤
//  │ Debug 3D 탭:                                            │
//  │   Three.js 3D 헤드 모델 실시간 회전 시각화               │
//  │   채널값 실시간 게이지                                    │
//  │   센서 상태 / ESP-NOW 통계                               │
//  └─────────────────────────────────────────────────────────┘
//
//  REST API:
//   GET  /api/status      → 실시간 상태 JSON (polling)
//   GET  /api/config      → 현재 설정 JSON
//   POST /api/config      → 설정 저장
//   GET  /api/scan        → ESP-NOW 주변 Backpack 스캔 시작
//   GET  /api/scan/result → 스캔 결과 목록 JSON
//   POST /api/bind        → MAC 바인드 확정
//   POST /api/unbind      → 바인드 해제 (브로드캐스트)
//   POST /api/center      → 센터 리셋
//   POST /api/ota         → OTA 업로드 (multipart/form-data)
// ═══════════════════════════════════════════════════════════════

class WebUI {
public:
    // WiFi AP 시작 + 웹서버 시작
    static void start();

    // WiFi AP + 웹서버 중지
    static void stop();

    static bool isRunning() { return _running; }

private:
    static void setupRoutes();
    static bool _running;
};
