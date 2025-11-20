#include "navigation2.h"
#include <string.h>

void navigation2_init(Navigation2* nav) {
    nav->running = true;
    nav->selected = 0;
    nav->uart_line[0] = '\0';
    nav->uart_new_data = false;
}

bool navigation2_running(Navigation2* nav) {
    return nav->running;
}

void navigation2_input(Navigation2* nav, InputEvent* event) {
    if(event->type == InputTypeShort) {
        switch(event->key) {
        case InputKeyBack:
            nav->running = false;
            break;

        case InputKeyUp:
            if(nav->selected > 0) nav->selected--;
            break;

        case InputKeyDown:
            if(nav->selected < 2) nav->selected++;     // placeholder menu size
            break;

        case InputKeyOk:
            // rien pour l'instant
            break;

        default:
            break;
        }
    }
}

void navigation2_update(Navigation2* nav, UartHandler* uart) {
    // Lire les données UART disponibles
    uint8_t byte;
    while(uart_handler_pop(uart, &byte)) {
        size_t len = strlen(nav->uart_line);

        if(len < sizeof(nav->uart_line) - 1) {
            nav->uart_line[len] = byte;
            nav->uart_line[len + 1] = '\0';
        }

        if(byte == '\n' || byte == '\r') {
            nav->uart_new_data = true;
        }
    }
}
