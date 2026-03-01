#include <Arduino.h>
#include "SharedState.h"
#include "config/Settings.h"
#include "sensor/SensorTask.h"
#include "sensor/Orientation.h"
#include "comms/CommsTask.h"
#include "comms/SBUSOutput.h"
#include "ui/LEDController.h"
#include "ui/ButtonTask.h"
#include "ui/WebUI.h"
#include "config/Config.h"

// ═══════════════════════════════════════════════════════════════
//  main.cpp  —  Phase 2
//  Settings(NVS) + LED + Button 통합
//  Phase 3: WebUI (WiFi AP + 바인드 UI + 3D 시각화)
// ═══════════════════════════════════════════════════════════════

SharedState gState;

// LED 업데이트 태스크 (20ms 주기)
static void ledTaskFn(void*) {
    while (true) {
        LEDController::update();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// LED 상태를 SharedState에 맞게 동기화
void syncLEDState() {
    static bool prevSensorOk  = false;
    static bool prevTracking  = true;
    static bool prevConnected = false;
    static bool prevWifiAp    = false;

    bool sensorOk, tracking, connected, wifiAp;
    CommsMode mode;

    if (!gState.lock()) return;
    sensorOk  = gState.sensorOk;
    tracking  = gState.tracking;
    connected = gState.espnowConnected;
    wifiAp    = gState.wifiApActive;
    mode      = gState.config.commsMode;
    gState.unlock();

    if (sensorOk == prevSensorOk && tracking == prevTracking &&
        connected == prevConnected && wifiAp == prevWifiAp) return;

    prevSensorOk  = sensorOk;
    prevTracking  = tracking;
    prevConnected = connected;
    prevWifiAp    = wifiAp;

    if      (wifiAp)            LEDController::setState(LEDState::WIFI_AP_ON);
    else if (!sensorOk)         LEDController::setState(LEDState::SENSOR_ERROR);
    else if (!tracking)         LEDController::setState(LEDState::OFF);
    else if (mode == COMMS_SBUS)LEDController::setState(LEDState::SBUS_ACTIVE);
    else if (connected)         LEDController::setState(LEDState::ESPNOW_CONNECTED);
    else                        LEDController::setState(LEDState::ESPNOW_SEARCHING);
}

// ══════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n");
    Serial.println("╔══════════════════════════════════╗");
    Serial.printf ("║  HeadTracker  FW %-15s║\n", FW_VERSION);
    Serial.printf ("║  HW: %-27s║\n", HW_VERSION);
    Serial.println("╚══════════════════════════════════╝");

    gState.init();

    LEDController::init();
    LEDController::setState(LEDState::BOOTING);

    Settings::init(gState);
    Serial.printf("[MAIN] Profile %d loaded\n", Settings::currentProfile() + 1);

    SensorTask::start();
    CommsTask::start();
    ButtonTask::start();
    xTaskCreate(ledTaskFn, "LEDTask", TASK_LED_STACK, nullptr, TASK_LED_PRIO, nullptr);

    Serial.println("[MAIN] All tasks started");
    Serial.println("[MAIN] Cmds: c=center  t=tracking  s=status  w=webui  m=mode  r=reset  h=help\n");
}

// ══════════════════════════════════════════════════════════════
void loop() {
    syncLEDState();

    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {

        case 'c':
            Orientation::resetCenter(gState);
            LEDController::flashWhite();
            Serial.println("[CMD] Center reset");
            break;

        case 't':
            if (gState.lock()) {
                gState.tracking = !gState.tracking;
                bool on = gState.tracking;
                gState.unlock();
                Serial.printf("[CMD] Tracking: %s\n", on ? "ON" : "OFF");
            }
            break;

        case 's': {
            if (!gState.lock()) break;
            float    pan    = gState.orientation.pan;
            float    tilt   = gState.orientation.tilt;
            float    roll   = gState.orientation.roll;
            uint16_t chPan  = gState.channels.pan();
            uint16_t chTilt = gState.channels.tilt();
            uint16_t chRoll = gState.channels.roll();
            bool     ok     = gState.sensorOk;
            bool     tr     = gState.tracking;
            bool     conn   = gState.espnowConnected;
            uint8_t  mac[6]; memcpy(mac, gState.config.espnowMac, 6);
            gState.unlock();

            CommsMode cm;
            if (gState.lock()) { cm = gState.config.commsMode; gState.unlock(); }

            Serial.println("\n── Status ──────────────────────────");
            Serial.printf("  Profile    : %d\n",   Settings::currentProfile() + 1);
            Serial.printf("  SensorOK   : %s\n",   ok   ? "YES"  : "NO");
            Serial.printf("  Tracking   : %s\n",   tr   ? "ON"   : "OFF");
            Serial.printf("  CommsMode  : %s\n",   cm == COMMS_SBUS ? "SBUS" : "ESP-NOW");
            if (cm == COMMS_ESPNOW) {
                Serial.printf("  ESP-NOW    : %s\n",   conn ? "CONN" : "DISC");
                Serial.printf("  Backpack   : %02X:%02X:%02X:%02X:%02X:%02X\n",
                              mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
            } else {
                Serial.printf("  SBUS TX    : GPIO%d (UART%d)\n", PIN_SBUS_TX, 1);
                Serial.printf("  SBUS TX/Fail: %lu / %lu\n",
                              SBUSOutput::getSentCount(), SBUSOutput::getFailedCount());
            }
            Serial.printf("  Pan/Tilt/Roll : %.1f / %.1f / %.1f deg\n", pan, tilt, roll);
            Serial.printf("  CH14/15/16    : %d / %d / %d\n", chPan, chTilt, chRoll);
            Serial.println("────────────────────────────────────\n");
            break;
        }

        case 'r':
            Settings::factoryReset(gState);
            Serial.println("[CMD] Factory reset done");
            break;

        case 'w':
            if (WebUI::isRunning()) {
                WebUI::stop();
                Serial.println("[CMD] WebUI stopped");
            } else {
                WebUI::start();
                Serial.println("[CMD] WebUI started → connect WiFi 'HeadTracker' → 192.168.4.1");
            }
            break;

        case 'm':
            if (gState.lock()) {
                CommsMode prev = gState.config.commsMode;
                gState.config.commsMode = (prev == COMMS_ESPNOW) ? COMMS_SBUS : COMMS_ESPNOW;
                CommsMode next = gState.config.commsMode;
                gState.unlock();
                Serial.printf("[CMD] CommsMode: %s → %s (restart to apply)\n",
                              prev == COMMS_SBUS ? "SBUS" : "ESP-NOW",
                              next == COMMS_SBUS ? "SBUS" : "ESP-NOW");
                Settings::save(gState);
            }
            break;

        case 'h':
            Serial.println("[CMD] c=center  t=tracking  s=status  w=webui  m=mode  r=factory_reset  h=help");
            break;

        default: break;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
}
