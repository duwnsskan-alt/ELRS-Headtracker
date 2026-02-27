#pragma once
#include <Arduino.h>
#include "SharedState.h"

// ═══════════════════════════════════════════════════════════════
//  CommsTask  —  50Hz로 채널 데이터를 ESP-NOW / SBUS로 전송
// ═══════════════════════════════════════════════════════════════

class CommsTask {
public:
    static void start();

private:
    static void taskFn(void* arg);
};
