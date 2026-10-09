// minimal Arduino stub for the PC display preview
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <climits>
#include <string>
#include <algorithm>
typedef unsigned int uint;
uint32_t millis();
class String : public std::string {
  public:
    String() {}
    String(const char *s) : std::string(s) {}
    String(const std::string &s) : std::string(s) {}
};
class IPAddress {
  public:
    IPAddress() : a{0,0,0,0} {}
    IPAddress(uint8_t x, uint8_t y, uint8_t z, uint8_t w) : a{x,y,z,w} {}
    String toString() const { char b[20]; snprintf(b, 20, "%d.%d.%d.%d", a[0],a[1],a[2],a[3]); return String(b); }
    bool operator!=(const IPAddress &o) const { return memcmp(a, o.a, 4) != 0; }
    explicit operator bool() const { return (a[0]|a[1]|a[2]|a[3]) != 0; }
    uint8_t a[4];
};
