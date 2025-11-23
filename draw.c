#include "draw.h"
#include "navigation2.h"
#include <stdio.h>

void draw_callback(Canvas* canvas, void* ctx) {
    Navigation2* nav = ctx;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);

    if(nav->state == NavStateMain) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 12, "Main Menu");
        canvas_draw_line(canvas, 0, 14, 127, 14);
        canvas_set_font(canvas, FontSecondary);

        const char* items[] = {"Scan WiFi", "Last Scan"};
        int y = 28;
        for(int i = 0; i < 2; i++) {
            if(nav->selected_index == i) canvas_draw_str(canvas, 2, y, ">");
            canvas_draw_str(canvas, 12, y, items[i]);
            y += 12;
        }

    } else if(nav->state == NavStateScanning) {
        canvas_draw_str(canvas, 30, 30, "Scanning...");
        if(nav->wifi_data.list_open) {
             char buf[32];
             snprintf(buf, sizeof(buf), "Found: %zu", nav->wifi_data.count);
             canvas_draw_str(canvas, 30, 42, buf);
        }

    } else if(nav->state == NavStateList) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 12, "Networks");
        canvas_draw_line(canvas, 0, 14, 127, 14);
        canvas_set_font(canvas, FontSecondary);

        int y = 26;
        size_t count = nav->wifi_data.count;
        for(int i = 0; i < 5; i++) {
            int idx = nav->scroll_offset + i;
            if(idx >= (int)count) break;

            if(idx == nav->selected_index) {
                canvas_draw_box(canvas, 0, y - 8, 128, 10);
                canvas_invert_color(canvas);
            }

            char buf[64];
            WifiNetwork* net = &nav->wifi_data.list[idx];
            // Format: "SSID (RSSI)"
            snprintf(buf, sizeof(buf), "%s (%d)", net->ssid, net->rssi);
            canvas_draw_str(canvas, 2, y, buf);

            if(idx == nav->selected_index) {
                canvas_invert_color(canvas);
            }
            y += 10;
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
    }
}
