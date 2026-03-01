# HeadTracker Firmware — Project Context

## 프로젝트 개요
ESP32-C6 + BNO085 기반 FPV 헤드트래커 펌웨어.
ELRS Backpack ESP-NOW으로 조종기에 Pan/Tilt/Roll 채널을 무선 전송한다.
오픈소스(MIT) + 하드웨어 판매 비즈니스 모델. Tindie → AliExpress 순 출시 예정.

---

## 하드웨어

| 부품 | 스펙 | 비고 |
|------|------|------|
| MCU | Seeed XIAO ESP32-C6 | Single-core RISC-V 160MHz, WiFi 6, BLE 5.3 |
| IMU | Adafruit BNO085 | 9DOF + 하드웨어 AHRS, 쿼터니언 출력, STEMMA QT |
| LED | XIAO 내장 WS2812 RGB | GPIO21 |
| 버튼1 | CENTER (BOOT) | GPIO0, 풀업, LOW=눌림, **확정** |
| 버튼2 | MODE | **⚠️ 미확정 — HW 도착 후 Config.h `PIN_BTN_MODE` 수정 필요** |
| SBUS | UART1 TX | GPIO17 (D6) |

---

## 통신 아키텍처

### Primary: ELRS Backpack ESP-NOW
- HeadTracker → ESP-NOW(2.4GHz) → TX 모듈 내장 Backpack 칩 → EdgeTX CH14/15/16
- WiFi 채널 6 고정 (ELRS Backpack 기본값)
- 바인드: WebUI Bind 탭에서 주변 스캔 → MAC 선택 → 저장 (버튼 없음)
- 바인드 전: 브로드캐스트(FF:FF:FF:FF:FF:FF), 바인드 후: 유니캐스트

### Secondary: SBUS 유선
- GPIO17 하드웨어 UART 반전, 100kbps
- ESP-NOW와 동시 출력 가능 (모드 독립)

### 미래 계획 (Phase 5)
- BLE PARA 프로토콜 (FrSky Tandem X20 등) — 현재 미구현, 추후 검토

---

## 채널 매핑

| 축 | ELRS 채널 | 배열 인덱스 | 범위 |
|----|----------|------------|------|
| Pan (Yaw) | CH14 | ch[13] | 172~1811, 중앙 992 |
| Tilt (Pitch) | CH15 | ch[14] | 172~1811, 중앙 992 |
| Roll | CH16 | ch[15] | 172~1811, 중앙 992 |

---

## 소프트웨어 구조

```
src/
├── main.cpp                 # 엔트리, 시리얼 명령(c/t/s/w/r/h), syncLEDState
├── SharedState.h            # 전역 상태 + FreeRTOS mutex (gState)
├── config/
│   ├── Config.h             # 핀/상수/튜닝 파라미터 — 여기서만 수정
│   └── Settings.cpp/h       # NVS 영구저장, 3개 프로파일
├── sensor/
│   ├── SensorTask.cpp/h     # BNO085 100Hz 읽기 FreeRTOS 태스크
│   └── Orientation.h        # 쿼터니언→오일러, 센터링, CRSF 변환
├── comms/
│   ├── ESPNowComms.cpp/h    # ESP-NOW 전송, 피어 관리, promiscuous 스캔
│   └── CommsTask.cpp/h      # 50Hz 전송 루프
├── crsf/
│   └── CRSF.h               # ELRS Backpack 패킷 빌더 (CRC-8 DVB-S2)
└── ui/
    ├── ButtonHandler.cpp/h  # 2버튼 이벤트 상태머신 (short/long/double)
    ├── ButtonTask.cpp/h     # 버튼 이벤트 → 액션 처리
    ├── LEDController.cpp/h  # WS2812 상태 표시 (BOOTING/ERROR/CONNECTED 등)
    └── WebUI.cpp/h          # WiFi AP + AsyncWebServer + SPA HTML
```

---

## FreeRTOS 태스크

| 태스크 | 우선순위 | 주기 | 역할 |
|--------|---------|------|------|
| SensorTask | 5 (최고) | 10ms | BNO085 읽기 → SharedState 기록 |
| CommsTask | 4 | 20ms | ESP-NOW/SBUS 50Hz 전송 |
| ButtonTask | 3 | 20ms | 버튼 이벤트 처리 |
| LEDTask | 1 | 20ms | LED 상태 업데이트 |
| WebUI (async) | - | - | AsyncWebServer (이벤트 루프) |

모든 SharedState 접근은 `gState.lock()` / `gState.unlock()` 필수.

---

## WebUI

접속: WiFi **"HeadTracker"** 연결 → `http://192.168.4.1`

| 탭 | 기능 |
|----|------|
| Settings | 각도 범위, 반전, 통신 모드, 프로파일(3개), 공장초기화 |
| Bind | 주변 ESP-NOW 스캔 → Backpack MAC 선택 → 바인드/해제 |
| Debug 3D | Three.js 실시간 3D 헤드 회전, Pan/Tilt/Roll 게이지, TX 통계 |

REST API: `GET /api/status` `GET/POST /api/config` `GET /api/scan` `GET /api/scan/result` `POST /api/bind` `POST /api/unbind` `POST /api/center` `POST /api/profile` `POST /api/reset` `POST /api/ota`

---

## 버튼 동작표

| 입력 | 동작 |
|------|------|
| CENTER 더블클릭 | 트래킹 ON/OFF 토글 |
| CENTER 1초 홀드 | 센터 리셋 (흰색 LED 플래시) |
| MODE 1초 홀드 | 프로파일 순환 (1→2→3→1) |
| MODE 2초 홀드 | 통신 모드 전환 (ESP-NOW ↔ SBUS) |
| CENTER+MODE 3초 | WiFi AP + WebUI 시작/종료 |

시리얼: `c`=센터리셋 `t`=트래킹토글 `s`=상태출력 `w`=WebUI토글 `r`=공장초기화 `h`=도움말

---

## LED 상태표

| 색상/패턴 | 상태 |
|----------|------|
| 노랑 느린 깜빡임 | 부팅/초기화 |
| 빨강 빠른 깜빡임 | 센서 오류 |
| 초록 빠른 깜빡임 | ESP-NOW 탐색 중 |
| 초록 solid | ESP-NOW 연결됨 |
| 파랑 solid | SBUS 모드 |
| 보라 solid | WiFi AP 활성 |
| 하늘 펄스(숨쉬기) | 바인드 모드 |
| 흰색 1회 플래시 | 센터 리셋 피드백 |
| 꺼짐 | 트래킹 OFF |

---

## 개발 현황

- ✅ **Phase 1** — 센서 + ESP-NOW 기본 전송 (브로드캐스트)
- ✅ **Phase 2** — 버튼 핸들러, LED, NVS 설정, WebUI (Settings/Bind/3D Debug)
- ⬜ **Phase 3** — SBUS 출력 구현 (`SBUSOutput.cpp`)
- ⬜ **Phase 4** — 필드 테스트 + 안정화
- ⬜ **Phase 5** — BLE PARA (FrSky) 추후 검토

---

## 미결 사항

- `PIN_BTN_MODE` (Config.h) — HW 도착 후 실제 GPIO 번호로 수정
- SBUS 출력 미구현 — `src/comms/SBUSOutput.cpp/h` 신규 작성 필요
- ESP-NOW WiFi 채널 자동 맞춤 — 현재 채널 6 하드코딩, ELRS bind_phrase 연동 미구현

---`

## 빌드 환경

```
Platform  : espressif32 (Arduino framework)
Board     : seeed_xiao_esp32c6
Monitor   : 115200 baud
Libraries : Adafruit BNO08x, Adafruit NeoPixel, ESPAsyncWebServer-esphome, ArduinoJson
Partition : partitions.csv (NVS 20KB 확보)
```

빌드: `pio run` / 업로드: `pio run -t upload` / 모니터: `pio device monitor`
