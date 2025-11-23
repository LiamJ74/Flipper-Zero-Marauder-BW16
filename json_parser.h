#pragma once
#include "wifi_types.h"
#include <stdbool.h>
#include <stdint.h>

bool json_parse_line(WifiApp* app, const char* line);
void uart_json_receive(WifiApp* app, uint8_t c);
