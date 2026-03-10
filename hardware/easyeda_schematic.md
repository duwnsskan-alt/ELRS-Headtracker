# HeadTracker C3 SuperMini — EasyEDA 회로도 가이드

## 컴포넌트 목록

| Ref | 부품명 | 스펙 | LCSC |
|-----|--------|------|------|
| U1 | ESP32-C3 SuperMini | 160MHz, WiFi+BLE, 4MB | — (직구) |
| U2 | BNO085 | 9DOF AHRS, I2C, STEMMA QT | — (Adafruit) |
| LED1 | WS2812B | 5V RGB NeoPixel (단일 픽셀) | C114586 |
| R1 | 저항 | 300Ω 1/4W | C57438 |
| R2 | 저항 | 10kΩ 1/4W (MODE 풀업) | C57438 |
| SW1 | 택트 스위치 | MODE 버튼, 6x6mm | C318884 |
| C1 | 커패시터 | 100nF 0.1μF 바이패스 | C1525 |
| C2 | 커패시터 | 10μF 탄탈/전해 (전원 안정화) | — |
| J1 | 핀헤더 | 2.54mm 2pin (SBUS 출력) | — |

---

## 핀 연결 네트리스트

### 전원
```
ESP32-C3 3V3  ──→  BNO085 VIN
ESP32-C3 3V3  ──→  WS2812B VCC  (또는 5V 직결 권장)
ESP32-C3 3V3  ──→  R2(10kΩ) pin1  [MODE 버튼 풀업]
ESP32-C3 GND  ──→  BNO085 GND
ESP32-C3 GND  ──→  WS2812B GND
ESP32-C3 GND  ──→  SW1 pin2WHt
ESP32-C3 GND  ──→  C1(100nF) pin2
ESP32-C3 GND  ──→  C2(10μF) GND
```ㅐ

### I2C — BNO085
```
ESP32-C3 GPIO6 (SDA)  ──→  BNO085 SDA
ESP32-C3 GPIO7 (SCL)  ──→  BNO085 SCL
BNO085 INT            ──   (미연결, NC)
BNO085 RST            ──   (미연결, NC)
```

### RGB LED — WS2812B
```
ESP32-C3 GPIO3  ──→  R1(300Ω) pin1
R1(300Ω) pin2   ──→  WS2812B DIN
WS2812B DOUT    ──   (미연결, 단일 픽셀)
```

### MODE 버튼
```
ESP32-C3 3V3  ──→  R2(10kΩ) pin1
R2(10kΩ) pin2 ──→  SW1 pin1  + ESP32-C3 GPIO10  [공통 노드]
SW1 pin2      ──→  ESP32-C3 GND
```
> 내부 풀업(INPUT_PULLUP) 사용 시 R2 생략 가능

### CENTER 버튼
```
GPIO9 = BOOT 버튼 (보드 내장, 별도 배선 불필요)
```

### SBUS 출력
```
ESP32-C3 GPIO4 (UART1 TX)  ──→  J1 pin1  [SBUS 신호]
ESP32-C3 GND               ──→  J1 pin2  [GND]
```
> SBUS는 100kbps 반전 UART. FC 수신기 SBUS IN 포트로 연결.

---

## EasyEDA 작업 순서

1. **easyeda.com** → Standard Edition → 새 프로젝트 → 새 회로도
2. **라이브러리 검색 (단축키 F)**:
   - `ESP32-C3` → SuperMini 없으면 `ESP32-C3-MINI-1` 사용 (핀 동일)
   - `BNO085` → 없으면 직접 심볼 생성: 6핀 (VIN, GND, SDA, SCL, INT, RST)
   - `WS2812B` → LCSC C114586 검색
   - `SW_Push` → 기본 라이브러리에 있음
   - `R` → 기본 저항 심볼
3. **배치 순서**: U1(MCU) 중앙 → U2(BNO085) 왼쪽 → LED1 오른쪽 → SW1 아래
4. **와이어 연결 (단축키 W)**: 위 네트리스트대로
5. **네트 레이블 (단축키 N)**: 긴 선은 레이블로 대체 (예: `SDA`, `SCL`, `GPIO4`)
6. **전원 심볼 (단축키 P)**: VCC(3.3V), GND 삽입
7. **저장 후 PCB 변환**: 우상단 `PCB` 버튼 → 나중에 KiCad 내보내기 가능

---

## ASCII 블록 다이어그램

```
        3V3──────────────────────────────┐
         │                               │
    ┌────┴────────────────────────────┐  │
    │       ESP32-C3 SuperMini        │  │
    │                                 │  │
    │  GPIO6 (SDA) ───────────────────┼──┼──→ BNO085 SDA
    │  GPIO7 (SCL) ───────────────────┼──┼──→ BNO085 SCL
    │                                 │  └──→ BNO085 VIN
    │  GPIO3 ──[R1 300Ω]─────────────┼──────→ WS2812B DIN
    │                                 │        WS2812B VCC ─── 5V
    │  GPIO4 (SBUS TX) ──────────────┼──────→ J1-1 (SBUS 출력)
    │                                 │
    │  GPIO9 [BOOT 내장] ← CENTER    │
    │  GPIO10 ──[R2 10k]─── 3V3     │
    │         ──── SW1 ──── GND     │
    │                                 │
    │  GND ──────────────────────────┼──────→ 공통 GND
    └─────────────────────────────────┘
```

---

## WS2812B 주의사항
- VCC는 가능하면 **5V 직결** (3.3V도 동작하나 밝기 저하)
- DIN 앞 **300Ω 직렬 저항** 필수 (반사파 보호)
- ESP32-C3 GPIO3는 3.3V 출력 → 5V WS2812B는 데이터 레벨에 민감
  → 레벨 시프터(74AHCT125) 추가 권장 (테스트 단계 생략 가능)
