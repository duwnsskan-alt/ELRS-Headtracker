// Compatibility stub for arduino-esp32 3.x
// Provides a minimal IPv6Address class to satisfy AsyncTCP-esphome

#pragma once
#include <stdint.h>
#include <string.h>

class IPv6Address {
public:
    IPv6Address() { memset(_addr, 0, sizeof(_addr)); }
    IPv6Address(const uint32_t* addr) { memcpy(_addr, addr, sizeof(_addr)); }
    IPv6Address(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
        _addr[0] = a; _addr[1] = b; _addr[2] = c; _addr[3] = d;
    }

    const uint32_t* data() const { return _addr; }

    // Allow static_cast<const uint32_t*>(ipv6addr) — required by AsyncTCP-esphome
    explicit operator const uint32_t*() const { return _addr; }

    bool operator==(const IPv6Address& other) const {
        return memcmp(_addr, other._addr, sizeof(_addr)) == 0;
    }
    bool operator!=(const IPv6Address& other) const { return !(*this == other); }

private:
    uint32_t _addr[4];
};
