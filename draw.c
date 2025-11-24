#include "draw.h"
#include "navigation2.h"
#include "app_context.h"
#include <stdio.h>

void draw_callback(Canvas* canvas, void* ctx) {
    App* app = ctx;
    if(furi_mutex_acquire(app->mutex, 200) != FuriStatusOk) return;

    Navigation2* nav = &app->nav;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);

    if(nav->state == NavStateMain) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 12, "Main Menu");
        canvas_draw_line(canvas, 0, 14, 127, 14);
        canvas_set_font(canvas, FontSecondary);

        const char* items[] = {"Scan WiFi", "Last Scan", "Debug / AT"};
        int y = 28;
        for(int i = 0; i < 3; i++) {
            if(nav->selected_index == i) canvas_draw_str(canvas, 2, y, ">");
            canvas_draw_str(canvas, 12, y, items[i]);
            y += 12;
        }

    } else if(nav->state == NavStateScanning) {
        canvas_draw_str(canvas, 30, 12, "Scanning...");

        char buf[64];
        // Show detected protocol
        const char* p_str = "UNK";
        if(nav->wifi_data.protocol == PROTO_REALTEK_AT) p_str = "RTL-AT";
        if(nav->wifi_data.protocol == PROTO_MARAUDER_CSV) p_str = "MARAUDER";
        if(nav->wifi_data.protocol == PROTO_MARAUDER_JSON) p_str = "JSON";

        snprintf(buf, sizeof(buf), "Proto: %s", p_str);
        canvas_draw_str(canvas, 30, 24, buf);

        snprintf(buf, sizeof(buf), "RX: %zu", nav->rx_count);
        canvas_draw_str(canvas, 30, 36, buf);

        if(nav->wifi_data.count > 0) {
             snprintf(buf, sizeof(buf), "Found: %zu", nav->wifi_data.count);
             canvas_draw_str(canvas, 30, 48, buf);
        }

        // Draw last UART line for debug
        canvas_set_font(canvas, FontSecondary);
        // Only draw first 20 chars to avoid mess
        snprintf(buf, sizeof(buf), "%.20s", nav->uart_line);
        canvas_draw_str(canvas, 2, 55, buf);

    } else if(nav->state == NavStateList) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 12, "Networks");
        canvas_draw_line(canvas, 0, 14, 127, 14);
        canvas_set_font(canvas, FontSecondary);

        size_t count = nav->wifi_data.count;
        if(count == 0) {
            canvas_draw_str(canvas, 10, 40, "No networks found.");
        } else {
            int y = 26;
            for(int i = 0; i < 5; i++) {
                int idx = nav->scroll_offset + i;
                if(idx >= (int)count) break;

                if(idx == nav->selected_index) {
                    canvas_draw_box(canvas, 0, y - 8, 128, 10);
                    canvas_invert_color(canvas);
                }

                char buf[128];
                WifiNetwork* net = &nav->wifi_data.list[idx];
                // Format: "SSID (RSSI)"
                snprintf(buf, sizeof(buf), "%.32s (%d)", net->ssid, net->rssi);
                canvas_draw_str(canvas, 2, y, buf);

                if(idx == nav->selected_index) {
                    canvas_invert_color(canvas);
                }
                y += 10;
            }
        }

    } else if(nav->state == NavStateOptions) {
        canvas_set_font(canvas, FontPrimary);
        if(nav->current_target) {
            canvas_draw_str(canvas, 2, 12, nav->current_target->ssid);
        } else {
             canvas_draw_str(canvas, 2, 12, "Target");
        }
        canvas_draw_line(canvas, 0, 14, 127, 14);
        canvas_set_font(canvas, FontSecondary);

        const char* opts[] = {"Deauth"};
        int y = 28;
        if(nav->selected_index == 0) canvas_draw_str(canvas, 2, y, ">");
        canvas_draw_str(canvas, 12, y, opts[0]);

    } else if(nav->state == NavStateDebug) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 12, "Terminal Mode");
        canvas_draw_line(canvas, 0, 14, 127, 14);

        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 2, 60, "OK:AT UP:Scan");

        // Draw last lines of log buffer
        // Simple: draw from end? Or wrap?
        // We have a big string in log_buffer.
        // Let's find the last few newlines to display just the tail.

        if(nav->log_len > 0) {
             // Heuristic: Count back 5 lines
             int lines = 0;
             char* ptr = &nav->log_buffer[nav->log_len - 1];
             while(ptr > nav->log_buffer && lines < 5) {
                 if(*ptr == '\n') lines++;
                 ptr--;
             }
             if(ptr != nav->log_buffer) ptr += 2; // Skip \n

             canvas_draw_str_aligned(canvas, 2, 20, AlignLeft, AlignTop, ptr);
        }
    }

    furi_mutex_release(app->mutex);
}
