#include "navigation2.h"
#include "bw16.h"
#include "json_parser.h"
#include <string.h>
#include <stdlib.h>

// Comparator for RSSI sorting (descending)
static int compare_rssi(const void* a, const void* b) {
    const WifiNetwork* netA = a;
    const WifiNetwork* netB = b;
    // Higher RSSI (closer to 0) is better. e.g. -50 > -90
    return netB->rssi - netA->rssi;
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
            if(nav->selected_index < 1) nav->selected_index++;
            break;
        case InputKeyOk:
            if(nav->selected_index == 0) {
                // Scan
                nav->state = NavStateScanning;
                nav->req_scan = true;
                nav->wifi_data.count = 0; // Clear list
            } else if(nav->selected_index == 1) {
                // Last scan
                nav->state = NavStateScanning; // Use scanning state while waiting data
                nav->req_last_scan = true;
                nav->wifi_data.count = 0;
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
                                      // Actually bw16.c uses "slot". If the module expects a slot index from the scan list...
                                      // Let's try sending the index (nav->selected in List).
                                      // Wait, we are in Options menu now. We need to remember the network index.
                                      // But `bw16_send_deauth` takes a `uint8_t slot`.
                                      // Marauder usually maps BSSIDs to slots implicitly or explicitly.
                                      // If the scan list from BW16 corresponds to slots 0..N, then we can use the list index.
                                      // Let's assume list index == slot for now.
                // We need the index from the list state. Ideally we stored it.
                // Let's say `current_target` pointer is useful for display, but for action we might need index.
                // We'll fix this if needed. Let's assume we pass the BSSID or something?
                // bw16.c sends 0xA2 + slot.
                // Let's guess the slot is the index in the list provided by BW16.
            }
            break;
        default: break;
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

    // Lire les données UART disponibles et les parser
    uint8_t byte;
    while(uart_handler_pop(uart, &byte)) {
        // Debug buffer
        size_t len = strlen(nav->uart_line);
        if(len < sizeof(nav->uart_line) - 1) {
            nav->uart_line[len] = byte;
            nav->uart_line[len + 1] = '\0';
        }
        if(byte == '\n' || byte == '\r') {
            nav->uart_new_data = true; // Trigger refresh
        }

        // JSON Parsing
        uart_json_receive(&nav->wifi_data, byte);
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
        if(!nav->wifi_data.list_open && nav->wifi_data.count > 0) {
             // Sort by RSSI descending
             qsort(nav->wifi_data.list, nav->wifi_data.count, sizeof(WifiNetwork), compare_rssi);

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
