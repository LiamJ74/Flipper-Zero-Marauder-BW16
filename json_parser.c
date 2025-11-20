#include "wifi_types.h"
#include <furi.h>
#include <stdlib.h>
#include <string.h>

static char recv_buffer[512];
static size_t recv_index = 0;

bool json_parse_line(WifiApp* app, const char* line) {
    if(!app || !line) return false;

    // Début d’une liste JSON
    if(strcmp(line, "[") == 0) {
        app->count = 0;
        app->list_open = true;
        FURI_LOG_I("5G", "Start list");
        return true;
    }

    // Fin d’une liste JSON
    if(strcmp(line, "]") == 0) {
        app->list_open = false;
        FURI_LOG_I("5G", "End list, %zu networks", app->count);
        return true;
    }

    // On parse uniquement si on est dans la liste
    if(!app->list_open) return false;

    // Vérification limite
    if(app->count >= MAX_NETWORKS) {
        FURI_LOG_E("5G", "Network list full");
        return false;
    }

    // On récupère le réseau courant
    WifiNetwork* net = &app->list[app->count];

    // ==== extraction SSID ====
    char* s = strstr(line, "\"ssid\":\"");
    if(!s) return false;
    s += 8;
    char* e = strchr(s, '"');
    if(!e) return false;
    size_t len = MIN(e - s, MAX_SSID_LEN - 1);
    memcpy(net->ssid, s, len);
    net->ssid[len] = 0;

    // ==== extraction BSSID ====
    s = strstr(line, "\"bssid\":\"");
    if(!s) return false;
    s += 9;
    e = strchr(s, '"');
    if(!e) return false;
    len = MIN(e - s, MAX_BSSID_LEN - 1);
    memcpy(net->bssid, s, len);
    net->bssid[len] = 0;

    // ==== extraction Channel ====
    s = strstr(line, "\"channel\":");
    if(!s) return false;
    s += 10;
    net->channel = atoi(s);

    // ==== extraction RSSI ====
    s = strstr(line, "\"rssi\":");
    if(!s) return false;
    s += 7;
    net->rssi = atoi(s);

    FURI_LOG_I("5G", "Added #%zu: %s (%s) Ch:%d RSSI:%d",
               app->count,
               net->ssid,
               net->bssid,
               net->channel,
               net->rssi);

    app->count++;

    return true;
}

void uart_json_receive(WifiApp* app, uint8_t c) {
    if(recv_index >= sizeof(recv_buffer) - 1) {
        recv_index = 0;
        return;
    }

    recv_buffer[recv_index++] = c;

    // Si fin de ligne → parse
    if(c == '\n' || c == '\r') {
        recv_buffer[recv_index - 1] = 0; // terminate
        json_parse_line(app, recv_buffer);
        recv_index = 0;
    }
}
