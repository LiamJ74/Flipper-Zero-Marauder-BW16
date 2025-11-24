#pragma once
#include <furi.h>

#define MAX_NETWORKS 128
#define MAX_SSID_LEN 64
#define MAX_BSSID_LEN 32

typedef enum {
    PROTO_UNKNOWN,
    PROTO_REALTEK_AT,
    PROTO_MARAUDER_CSV,
    PROTO_MARAUDER_JSON,
    PROTO_GENERIC_SCAN
} WifiProtocol;

typedef struct {
    char ssid[MAX_SSID_LEN];
    char bssid[MAX_BSSID_LEN];
    int channel;
    int rssi;
} WifiNetwork;

typedef struct {
    WifiNetwork list[MAX_NETWORKS];
    size_t count;
    bool scanning;
    bool list_open;
    WifiProtocol protocol;
} WifiApp;
