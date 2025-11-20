#pragma once

#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/canvas.h>

typedef struct {
    int selected;
} DrawState;

void draw_init(DrawState* state);
void draw_callback(Canvas* canvas, void* ctx);
