#pragma once
#include <stdint.h>
#include <furi.h>
#include <furi_hal.h>

typedef struct {
    FuriHalSerialHandle* serial;
    uint8_t buffer[256];
    size_t head;
    size_t tail;
} UartHandler;

void uart_handler_init(UartHandler* handler);
void uart_handler_push(UartHandler* handler, uint8_t byte);
void uart_handler_set_serial(UartHandler* handler, FuriHalSerialHandle* serial);
