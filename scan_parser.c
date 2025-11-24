#include "scan_parser.h"
#include <furi.h>
#include <stdlib.h>
#include <string.h>

static char recv_buffer[1024];
static size_t recv_index = 0;

// --- Parsers ---

static void parse_realtek_at(WifiApp* app, const char* line) {
    // Format: +WSCAN:<ssid>,<bssid>,<rssi>,<channel>
    // Example: +WSCAN:MyAP,00:11:22:33:44:55,-60,6
    if(app->count >= MAX_NETWORKS) return;

    char* ptr = (char*)line;
    if(strncmp(ptr, "+WSCAN:", 7) == 0) ptr += 7;
    else return;

    WifiNetwork* net = &app->list[app->count];

    // SSID (can contain commas, so we assume BSSID is fixed format or parse backwards?
    // Realtek AT usually puts SSID first. Let's assume SSID ends at first comma for now,
    // OR we scan for the MAC address pattern.
    // Simple approach: strchr(ptr, ',')

    char* comma = strchr(ptr, ',');
    if(!comma) return;
    size_t ssid_len = MIN((size_t)(comma - ptr), MAX_SSID_LEN - 1);
    memcpy(net->ssid, ptr, ssid_len);
    net->ssid[ssid_len] = 0;

    ptr = comma + 1; // BSSID
    comma = strchr(ptr, ',');
    if(!comma) return;
    size_t bssid_len = MIN((size_t)(comma - ptr), MAX_BSSID_LEN - 1);
    memcpy(net->bssid, ptr, bssid_len);
    net->bssid[bssid_len] = 0;

    ptr = comma + 1; // RSSI
    comma = strchr(ptr, ',');
    if(!comma) return;
    net->rssi = atoi(ptr);

    ptr = comma + 1; // Channel
    net->channel = atoi(ptr);

    app->count++;
}

static void parse_marauder_csv(WifiApp* app, const char* line) {
    // Format: SCANRESULT,<MAC>,<RSSI>,<CH>,<SSID>
    if(app->count >= MAX_NETWORKS) return;

    char* ptr = (char*)line + 11; // Skip "SCANRESULT,"
    WifiNetwork* net = &app->list[app->count];

    // MAC
    char* comma = strchr(ptr, ',');
    if(!comma) return;
    size_t mac_len = MIN((size_t)(comma - ptr), MAX_BSSID_LEN - 1);
    memcpy(net->bssid, ptr, mac_len);
    net->bssid[mac_len] = 0;

    ptr = comma + 1; // RSSI
    comma = strchr(ptr, ',');
    if(!comma) return;
    net->rssi = atoi(ptr);

    ptr = comma + 1; // Channel
    comma = strchr(ptr, ',');
    if(!comma) return;
    net->channel = atoi(ptr);

    ptr = comma + 1; // SSID
    size_t ssid_len = strlen(ptr);
    if(ssid_len >= MAX_SSID_LEN) ssid_len = MAX_SSID_LEN - 1;
    memcpy(net->ssid, ptr, ssid_len);
    net->ssid[ssid_len] = 0;

    app->count++;
}

static void parse_marauder_json_line(WifiApp* app, const char* line) {
    // Basic JSON line parser for [ ... ] blocks
    // Only extracting inside list
    if(strstr(line, "[") != NULL) {
        app->count = 0;
        app->list_open = true;
        return;
    }
    if(strstr(line, "]") != NULL) {
        app->list_open = false;
        return;
    }
    if(!app->list_open) return;
    if(app->count >= MAX_NETWORKS) return;

    WifiNetwork* net = &app->list[app->count];

    // "ssid":"..."
    char* s = strstr(line, "\"ssid\":\"");
    if(!s) return;
    s += 8;
    char* e = strchr(s, '"');
    if(!e) return;
    size_t len = MIN((size_t)(e - s), MAX_SSID_LEN - 1);
    memcpy(net->ssid, s, len);
    net->ssid[len] = 0;

    // "rssi":
    s = strstr(line, "\"rssi\":");
    if(s) net->rssi = atoi(s + 7);

    // "channel":
    s = strstr(line, "\"channel\":");
    if(s) net->channel = atoi(s + 10);

    app->count++;
}

// --- Detection ---

static WifiProtocol detect_protocol(const char* line) {
    if(strncmp(line, "+WSCAN:", 7) == 0) return PROTO_REALTEK_AT;
    if(strncmp(line, "SCANRESULT,", 11) == 0) return PROTO_MARAUDER_CSV;
    if(strstr(line, "[") != NULL && strstr(line, "{") != NULL) return PROTO_MARAUDER_JSON;
    // Additional signatures
    if(strncmp(line, "AP ", 3) == 0) return PROTO_GENERIC_SCAN;
    if(strncmp(line, "SCAN:", 5) == 0) return PROTO_GENERIC_SCAN;

    return PROTO_UNKNOWN;
}

// --- Main Receiver ---

void uart_scan_receive(WifiApp* app, uint8_t c) {
    if(recv_index >= sizeof(recv_buffer) - 1) {
        recv_index = 0;
        return;
    }

    recv_buffer[recv_index++] = c;
    recv_buffer[recv_index] = '\0';

    // Triggers: Newline OR ']' (for JSON without newline)
    bool trigger = (c == '\n' || c == '\r');

    // Check JSON closing bracket if protocol is potentially JSON
    if(c == ']') trigger = true;

    if(trigger) {
        // Trim
        size_t len = recv_index;
        while(len > 0 && (recv_buffer[len-1] == '\n' || recv_buffer[len-1] == '\r')) {
            recv_buffer[--len] = 0;
        }

        if(len == 0) {
            recv_index = 0;
            return;
        }

        // Detection
        if(app->protocol == PROTO_UNKNOWN) {
            app->protocol = detect_protocol(recv_buffer);
        }

        // Parsing
        switch(app->protocol) {
            case PROTO_REALTEK_AT:
                if(strncmp(recv_buffer, "+WSCAN:", 7) == 0) parse_realtek_at(app, recv_buffer);
                if(strcmp(recv_buffer, "OK") == 0) app->scanning = false;
                break;

            case PROTO_MARAUDER_CSV:
                if(strncmp(recv_buffer, "SCANRESULT,", 11) == 0) parse_marauder_csv(app, recv_buffer);
                if(strncmp(recv_buffer, "LIST,", 5) == 0) app->scanning = false;
                break;

            case PROTO_MARAUDER_JSON:
                parse_marauder_json_line(app, recv_buffer);
                if(!app->list_open && app->count > 0) app->scanning = false;
                break;

            default:
                // Keep trying to detect if we are receiving data
                if(app->protocol == PROTO_UNKNOWN) {
                    // maybe generic
                }
                break;
        }

        recv_index = 0;
    }
}
