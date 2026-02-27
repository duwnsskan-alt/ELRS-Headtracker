# Changelog

## [0.2.0-alpha] — Phase 2 (2026-02-27)

### 추가된 기능

#### UI 모듈 (`src/ui/`)
- **LEDController** — WS2812 RGB LED 상태 머신
  - 상태: BOOTING(노랑 깜빡), SENSOR_ERROR(빨강 깜빡), ESPNOW_SEARCHING(초록 깜빡), ESPNOW_CONNECTED(초록), SBUS_ACTIVE(파랑), WIFI_AP(보라), OFF
  - 흰색 1회 플래시: 센터 리셋 피드백
- **ButtonHandler** — 2버튼 이벤트 상태머신
  - 짧은 클릭 / 더블클릭 / 1초·2초·3초 홀드 구분
  - 소프트웨어 디바운스 내장
- **ButtonTask** — 버튼 이벤트 → 액션 매핑
  - CENTER 더블클릭: 트래킹 ON/OFF
  - CENTER 1초 홀드: 센터 리셋
  - MODE 1초 홀드: 프로파일 순환 (1→2→3→1)
  - MODE 2초 홀드: 통신 모드 전환 (ESP-NOW ↔ SBUS)
  - CENTER+MODE 3초: WiFi AP + WebUI 시작/종료
- **WebUI** — WiFi AP 모드 + AsyncWebServer (817줄)
  - SSID: `HeadTracker` / 접속: `http://192.168.4.1`
  - **Settings 탭**: 각도 범위, 채널 반전, 통신 모드, 3개 프로파일 전환, 공장초기화
  - **Bind 탭**: 주변 ESP-NOW 기기 스캔 → Backpack MAC 선택 → 바인드/해제
  - **Debug 3D 탭**: Three.js 실시간 3D 헤드 시각화, Pan/Tilt/Roll 게이지, TX 통계

#### 설정 영구저장 (`src/config/`)
- **Settings** — NVS 기반 영구 설정 저장
  - 프로파일 3개 독립 저장 (각도 범위, 반전, 통신 모드, Backpack MAC)
  - 공장초기화 (`r` 시리얼 명령 또는 WebUI)
  - 부팅 시 자동 로드

#### ESP-NOW Backpack 스캔 (`src/comms/ESPNowComms.cpp`)
- Promiscuous 모드로 주변 ESP-NOW 기기 자동 탐색 (5초)
- 탐색된 MAC + RSSI 목록을 WebUI Bind 탭에 표시
- `espnow_start_scan()` / `espnow_get_scan_results()` / `espnow_set_peer()` API 추가

### 변경된 사항

| 항목 | Phase 1 | Phase 2 |
|------|---------|---------|
| 펌웨어 버전 | `0.1.0-alpha` | `0.2.0-alpha` |
| ESP-NOW 채널 | 0 (자동) | 6 (ELRS Backpack 기본값 고정) |
| 버튼 모듈 위치 | `src/button/` | `src/ui/` (UI 통합) |
| 설정 저장 | 하드코딩 (재부팅 시 초기화) | NVS 영구저장 |
| 시리얼 명령 | `c` `t` `s` `h` | `c` `t` `s` `w` `r` `h` 추가 |
| 상태 출력(`s`) | 기본 정보 | Backpack MAC, 프로파일 번호 포함 |
| `main.cpp` | Phase 1 진입점 | `syncLEDState()` + 태스크 통합 |

### 추가된 라이브러리 (`platformio.ini`)
- `adafruit/Adafruit NeoPixel ^1.12.0` — WS2812 LED
- `esphome/ESPAsyncWebServer-esphome ^3.3.0` — 비동기 웹서버
- `bblanchon/ArduinoJson ^7.2.0` — REST API JSON 처리

### 구조 변경
```
삭제: src/button/ButtonTask.cpp, src/button/ButtonTask.h
추가: src/ui/  (ButtonHandler, ButtonTask, LEDController, WebUI)
추가: src/config/Settings.cpp, src/config/Settings.h
추가: CLAUDE.md  (Claude AI 컨텍스트 파일)
```

### 미결 사항 (Phase 3 진행 전 확인 필요)
- `PIN_BTN_MODE` — HW 도착 후 실제 GPIO 번호로 `Config.h` 수정
- SBUS 출력 미구현 — `src/comms/SBUSOutput.cpp/h` 신규 작성 필요

---

## [0.1.0-alpha] — Phase 1 (2026-02-27)

### 최초 구현
- BNO085 100Hz 센서 읽기 (SensorTask)
- 쿼터니언 → 오일러 변환, 센터링, CRSF 채널 변환 (Orientation.h)
- ESP-NOW 브로드캐스트로 ELRS Backpack에 CH14/15/16 전송 (CommsTask, 50Hz)
- CRSF 패킷 빌더 with CRC-8 DVB-S2 (CRSF.h)
- FreeRTOS 멀티태스크 구조 + SharedState mutex
- 시리얼 명령: `c`=센터리셋, `t`=트래킹토글, `s`=상태출력, `h`=도움말
