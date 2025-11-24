#include "bw16.h"
#include "uart_handler.h"
#include <furi_hal.h>
#include <string.h>

extern UartHandler global_uart; // si tu veux, on peut passer l’UART en paramètre

static void bw16_send_raw(FuriHalSerialHandle* handle, const uint8_t* data, uint8_t len) {
    if(!handle) return;
    furi_hal_serial_tx(handle, data, len);
}

void bw16_send_scan_request(FuriHalSerialHandle* handle) {
    const uint8_t pkt[] = {0xA1, 0x01};
    bw16_send_raw(handle, pkt, sizeof(pkt));
}

void bw16_send_deauth(FuriHalSerialHandle* handle, uint8_t slot) {
    const uint8_t pkt[] = {0xA2, slot};
    bw16_send_raw(handle, pkt, sizeof(pkt));
}

void bw16_send_get_last_scan(FuriHalSerialHandle* handle) {
    const uint8_t pkt[] = {0xA3, 0x00};
    bw16_send_raw(handle, pkt, sizeof(pkt));
}

void bw16_send_string(FuriHalSerialHandle* handle, const char* str) {
    if(!handle || !str) return;
    furi_hal_serial_tx(handle, (const uint8_t*)str, strlen(str));
}
