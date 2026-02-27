#pragma once
#include <Arduino.h>

class ButtonTask {
public:
    static void start();
private:
    static void taskFn(void* arg);
};
