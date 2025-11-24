#pragma once

#include <furi.h>
#include <input/input.h>
#include "uart_handler.h"
#include "wifi_types.h"

typedef enum {
    NavStateMain,
    NavStateScanning,
    NavStateList,
    NavStateOptions,
    NavStateDebug,
} NavState;

typedef struct {
    bool running;
    NavState state;

    // Selection index
    int selected_index;
    int scroll_offset;

    // Data
    WifiApp wifi_data;
    WifiNetwork* current_target;

    // UART buffer line for legacy/debug
    char uart_line[128];
    bool uart_new_data;

    // Debug Log
    char log_buffer[1024];
    size_t log_len;

    // Action Flags
    bool req_scan;
    bool req_last_scan;
    bool req_deauth;

    // Debug requests
    bool req_dbg_at;
    bool req_dbg_scan;

    uint8_t deauth_slot;
    uint32_t scan_start_tick;
    size_t rx_count;
} Navigation2;

void navigation2_init(Navigation2* nav);
void navigation2_input(Navigation2* nav, InputEvent* event);
void navigation2_update(Navigation2* nav, UartHandler* uart);
bool navigation2_running(Navigation2* nav);
