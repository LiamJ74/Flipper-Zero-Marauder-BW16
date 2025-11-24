#pragma once
#include <furi.h>
#include <furi_hal.h>
#include "navigation2.h"
#include "uart_handler.h"

typedef struct {
    Navigation2 nav;
    UartHandler uart;
    FuriMutex* mutex;
    FuriThread* thread;
    FuriHalSerialHandle* serial;
    FuriMessageQueue* event_queue;
} App;
