#pragma once
#include <stdint.h>
#include <furi_hal_serial.h>

void bw16_send_scan_request(FuriHalSerialHandle* handle);
void bw16_send_deauth(FuriHalSerialHandle* handle, uint8_t slot);
void bw16_send_get_last_scan(FuriHalSerialHandle* handle);
void bw16_send_string(FuriHalSerialHandle* handle, const char* str);
