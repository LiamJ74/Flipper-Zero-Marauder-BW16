#include "uart_handler.h"

void uart_handler_init(UartHandler* handler) {
    handler->serial = NULL;
    handler->head = 0;
    handler->tail = 0;
}

void uart_handler_push(UartHandler* handler, uint8_t byte) {
    size_t next = (handler->head + 1) % sizeof(handler->buffer);
    if(next != handler->tail) {
        handler->buffer[handler->head] = byte;
        handler->head = next;
    }
}

bool uart_handler_pop(UartHandler* handler, uint8_t* byte) {
    if(handler->head == handler->tail) {
        return false;
    }
    *byte = handler->buffer[handler->tail];
    handler->tail = (handler->tail + 1) % sizeof(handler->buffer);
    return true;
}

void uart_handler_set_serial(UartHandler* handler, FuriHalSerialHandle* serial) {
    handler->serial = serial;
    if(serial) {
        // furi_hal_serial_async_rx_start should be called by the owner of the handle
        // because it requires a callback.
    }
}
