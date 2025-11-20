#pragma once
#include <stdint.h>

typedef struct {
    char ssid[32];
    char ssid_alt[32];
    uint8_t channel;
    int8_t rssi;
    uint8_t flags;
} WifiEntry;

typedef struct {
    uint8_t ui_state;
    uint8_t selected;
    uint16_t pad;
    uint32_t net_count;
    uint8_t multi_attack_enabled;
    WifiEntry entries[64];
} AppState;
