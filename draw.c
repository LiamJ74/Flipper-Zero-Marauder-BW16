#include "draw.h"

void draw_init(DrawState* state) {
    state->selected = 0;
}

void draw_callback(Canvas* canvas, void* ctx) {
    DrawState* state = ctx;

    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "5G Rebuild");

    canvas_set_font(canvas, FontSecondary);

    char buf[32];
    snprintf(buf, sizeof(buf), "Select: %d", state->selected);
    canvas_draw_str(canvas, 2, 30, buf);

    canvas_draw_line(canvas, 0, 0, 127, 0);
}
