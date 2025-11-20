#pragma once

#include <furi.h>
#include <input/input.h>
#include "uart_handler.h"

typedef struct {
    bool running;
    uint8_t selected;
    char uart_line[128];
    bool uart_new_data;
    bool req_scan;
    bool req_last_scan;
    bool req_deauth;
} Navigation2;

void navigation2_init(Navigation2* nav);
void navigation2_input(Navigation2* nav, InputEvent* event);
void navigation2_update(Navigation2* nav, UartHandler* uart);
bool navigation2_running(Navigation2* nav);
