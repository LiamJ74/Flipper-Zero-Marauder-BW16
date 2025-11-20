#include "bw16.h"
#include "uart_handler.h"
#include <furi_hal.h>

extern UartHandler global_uart; // si tu veux, on peut passer l’UART en paramètre

static void bw16_send_raw(const uint8_t* data, uint8_t len) {
    FuriHalSerialHandle* handle = furi_hal_serial_get(FuriHalSerialIdUsart);
    if(!handle) return;

    for(uint8_t i = 0; i < len; i++) {
        furi_hal_serial_tx(handle, data[i]);
    }
}

void bw16_send_scan_request(void) {
    const uint8_t pkt[] = {0xA1, 0x01};
    bw16_send_raw(pkt, sizeof(pkt));
}

void bw16_send_deauth(uint8_t slot) {
    const uint8_t pkt[] = {0xA2, slot};
    bw16_send_raw(pkt, sizeof(pkt));
}

void bw16_send_get_last_scan(void) {
    const uint8_t pkt[] = {0xA3, 0x00};
    bw16_send_raw(pkt, sizeof(pkt));
}
