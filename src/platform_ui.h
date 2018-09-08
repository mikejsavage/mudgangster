#pragma once

#include "ui.h"

void platform_ui_init();
void platform_ui_term();

void platform_fill_rect( int left, int top, int width, int height, Colour colour, bool bold );
void platform_draw_char( int left, int top, char c, Colour colour, bool bold, bool force_bold_font );
void platform_make_dirty( int left, int top, int width, int height );

void platform_set_clipboard( const char * str, size_t len );
