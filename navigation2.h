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

    // Action Flags
    bool req_scan;
    bool req_last_scan;
    bool req_deauth;
    uint8_t deauth_slot;
    uint32_t scan_start_tick;
} Navigation2;

void navigation2_init(Navigation2* nav);
void navigation2_input(Navigation2* nav, InputEvent* event);
void navigation2_update(Navigation2* nav, UartHandler* uart);
bool navigation2_running(Navigation2* nav);
