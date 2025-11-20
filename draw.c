#include "draw.h"
#include "navigation2.h"
#include <stdio.h>

void draw_callback(Canvas* canvas, void* ctx) {
    Navigation2* nav = ctx;

    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "5G Rebuild");
    canvas_draw_line(canvas, 0, 14, 127, 14);

    canvas_set_font(canvas, FontSecondary);

    const char* menu_items[] = {"Scan WiFi", "Last Scan", "Deauth"};
    int y = 28;

    for(int i = 0; i < 3; i++) {
        if(nav->selected == i) {
            canvas_draw_str(canvas, 2, y, ">");
        }
        canvas_draw_str(canvas, 12, y, menu_items[i]);
        y += 12;
    }

    // Show status/last line
    if(nav->uart_new_data) {
       canvas_draw_str(canvas, 2, 60, "RX Data!");
    }
}
