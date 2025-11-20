#pragma once
#include <stdint.h>

void bw16_send_scan_request(void);
void bw16_send_deauth(uint8_t slot);
void bw16_send_get_last_scan(void);
