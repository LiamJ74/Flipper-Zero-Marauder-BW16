#include "navigation2.h"
#include "bw16.h"
#include "json_parser.h"
#include <string.h>

// Simple sorting helper (Bubble Sort for simplicity on small array)
static void sort_networks_by_rssi(WifiNetwork* list, size_t count) {
    if(count < 2) return;

    for(size_t i = 0; i < count - 1; i++) {
        for(size_t j = 0; j < count - i - 1; j++) {
            // Higher RSSI (closer to 0) comes first. e.g. -50 > -90
            if(list[j].rssi < list[j + 1].rssi) {
                WifiNetwork temp = list[j];
                list[j] = list[j + 1];
                list[j + 1] = temp;
            }
        }
    }
}

void navigation2_init(Navigation2* nav) {
    memset(nav, 0, sizeof(Navigation2));
    nav->running = true;
    nav->state = NavStateMain;
    nav->selected_index = 0;
    nav->wifi_data.count = 0;
    nav->wifi_data.list_open = false;
}

bool navigation2_running(Navigation2* nav) {
    return nav->running;
}

void navigation2_input(Navigation2* nav, InputEvent* event) {
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return;

    switch(nav->state) {
    case NavStateMain:
        switch(event->key) {
        case InputKeyBack: nav->running = false; break;
        case InputKeyUp:
            if(nav->selected_index > 0) nav->selected_index--;
            break;
        case InputKeyDown:
            if(nav->selected_index < 2) nav->selected_index++;
            break;
        case InputKeyOk:
            if(nav->selected_index == 0) {
                // Scan
                nav->state = NavStateScanning;
                nav->req_scan = true;
                nav->wifi_data.count = 0; // Clear list
                nav->scan_start_tick = furi_get_tick();
            } else if(nav->selected_index == 1) {
                // Last scan
                nav->state = NavStateScanning; // Use scanning state while waiting data
                nav->req_last_scan = true;
                nav->wifi_data.count = 0;
                nav->scan_start_tick = furi_get_tick();
            } else if(nav->selected_index == 2) {
                // Debug / Terminal
                nav->state = NavStateDebug;
                nav->log_len = 0;
                nav->log_buffer[0] = '\0';
            }
            break;
        default: break;
        }
        break;

    case NavStateScanning:
        if(event->key == InputKeyBack) {
            nav->state = NavStateMain;
        }
        break;

    case NavStateList:
        switch(event->key) {
        case InputKeyBack:
            nav->state = NavStateMain;
            nav->selected_index = 0;
            break;
        case InputKeyUp:
            if(nav->selected_index > 0) {
                nav->selected_index--;
                if(nav->selected_index < nav->scroll_offset) {
                    nav->scroll_offset = nav->selected_index;
                }
            }
            break;
        case InputKeyDown:
            if(nav->selected_index < (int)nav->wifi_data.count - 1) {
                nav->selected_index++;
                if(nav->selected_index >= nav->scroll_offset + 5) {
                    nav->scroll_offset = nav->selected_index - 4;
                }
            }
            break;
        case InputKeyOk:
            if(nav->wifi_data.count > 0) {
                nav->current_target = &nav->wifi_data.list[nav->selected_index];
                nav->state = NavStateOptions;
                nav->selected_index = 0;
            }
            break;
        default: break;
        }
        break;

    case NavStateOptions:
        switch(event->key) {
        case InputKeyBack:
            nav->state = NavStateList;
            nav->selected_index = 0;
            break;
        case InputKeyOk:
            if(nav->selected_index == 0) {
                // Deauth
                nav->req_deauth = true;
                nav->deauth_slot = 0; // The logic for slots is complex, for now assuming direct deauth or need slot mapping.
            }
            break;
        default: break;
        }
        break;

    case NavStateDebug:
        if(event->key == InputKeyBack) {
            nav->state = NavStateMain;
        } else if(event->key == InputKeyOk) {
             nav->req_dbg_at = true;
        } else if(event->key == InputKeyUp) {
             nav->req_dbg_scan = true;
        }
        break;
    }
}

void navigation2_update(Navigation2* nav, UartHandler* uart) {
    if(nav->req_scan) {
        bw16_send_scan_request(uart->serial);
        nav->req_scan = false;
    }
    if(nav->req_last_scan) {
        bw16_send_get_last_scan(uart->serial);
        nav->req_last_scan = false;
    }
    if(nav->req_deauth) {
        // We need to find the index of current_target in the list to use as slot
        // Or if we stored the index.
        // Quick hack: find index by pointer arithmetic
        int slot = nav->current_target - nav->wifi_data.list;
        if(slot >= 0 && slot < (int)nav->wifi_data.count) {
             bw16_send_deauth(uart->serial, (uint8_t)slot);
        }
        nav->req_deauth = false;
        // Maybe go back to list?
        nav->state = NavStateList;
    }

    if(nav->req_dbg_at) {
        bw16_send_string(uart->serial, "AT\r\n");
        nav->req_dbg_at = false;
    }
    if(nav->req_dbg_scan) {
        // Try generic Realtek/B&T scan command
        bw16_send_string(uart->serial, "AT+WSCAN\r\n");
        nav->req_dbg_scan = false;
    }

    // Lire les données UART disponibles et les parser
    uint8_t byte;
    while(uart_handler_pop(uart, &byte)) {
        nav->rx_count++;

        // Debug buffer
        size_t len = strlen(nav->uart_line);
        if(byte == '\n' || byte == '\r') {
            // New line handling
            nav->uart_new_data = true;
            // Clear line for next packet?
            // If we clear it here, we might miss displaying it.
            // Better to clear it at start of new line.
            // For now, let's just append and clear if full or on newline.
            if(len > 0) memset(nav->uart_line, 0, sizeof(nav->uart_line));
        } else {
            if(len < sizeof(nav->uart_line) - 1) {
                nav->uart_line[len] = byte;
                nav->uart_line[len + 1] = '\0';
            }
        }

        // JSON Parsing
        uart_json_receive(&nav->wifi_data, byte);

        // Debug Log append
        if(nav->state == NavStateDebug) {
            if(nav->log_len < sizeof(nav->log_buffer) - 2) {
                nav->log_buffer[nav->log_len] = byte;
                nav->log_buffer[nav->log_len + 1] = '\0';
                nav->log_len++;
            } else {
                // Scroll: Shift buffer left by half
                size_t shift = sizeof(nav->log_buffer) / 2;
                memmove(nav->log_buffer, nav->log_buffer + shift, nav->log_len - shift);
                nav->log_len -= shift;
                nav->log_buffer[nav->log_len] = byte;
                nav->log_buffer[nav->log_len + 1] = '\0';
                nav->log_len++;
            }
        }
    }

    // Check if scan finished (list_open went true then false)
    // The parser `uart_json_receive` calls `json_parse_line`.
    // `json_parse_line` sets `list_open` to true on '[' and false on ']'.
    // If we were scanning and `list_open` becomes false (and count > 0 or we received the closing bracket),
    // we should transition to List state.
    // However, `uart_json_receive` is void. We can check `nav->wifi_data.list_open` state changes.
    // This is tricky because `uart_handler_pop` loop might process the whole list in one update.
    // We can rely on `nav->wifi_data.list_open` being false AND `nav->state == NavStateScanning` AND we have processed data?
    // Actually, `json_parser` logs "End list".

    // Simple logic: If we are in Scanning state, and we see the list is closed (after being opened?), or just check if we have items.
    // But `list_open` is false initially.
    // We need a way to know if we just finished receiving a list.
    // Let's assume if we are scanning, and `list_open` is false, and we have > 0 items, we are done?
    // No, initially 0 items.

    // Better: if `NavStateScanning`, and `wifi_data.count > 0` and `!list_open`, then we are done.
    // But what if the list is empty?
    // The parser handles state. Maybe we just wait for a timeout or user interaction if list is empty.

    if(nav->state == NavStateScanning) {
        bool timeout = (furi_get_tick() - nav->scan_start_tick) > 10000;
        // Check if list closed (scan finished) OR timeout
        // Also ensure we at least started receiving something (list_open was true at some point?)
        // The parser logic sets list_open=true on '[' and false on ']'.
        // If we receive "[]", list_open goes true then false.
        // We can check if list_open is false. But initially it is false too.
        // We need a flag "scan_started_receiving" maybe?
        // Or just rely on timeout if list is empty.
        // But if we received ']', list_open is false.
        // Let's assume if count > 0 and !list_open, we are good.
        // OR if timeout.

        // Improvement: we can check if we received ANY data?
        // Let's just use timeout for "0 networks" case, OR if we detect the end of list.
        // Ideally json_parser sets a "finished" flag.
        // But for now:

        if((!nav->wifi_data.list_open && nav->wifi_data.count > 0) || timeout) {
             // Sort by RSSI descending
             sort_networks_by_rssi(nav->wifi_data.list, nav->wifi_data.count);

             // Keep top 10
             if(nav->wifi_data.count > 10) {
                 nav->wifi_data.count = 10;
             }

             nav->state = NavStateList;
             nav->selected_index = 0;
             nav->scroll_offset = 0;
        }
    }
}
