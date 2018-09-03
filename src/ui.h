#pragma once

#include "common.h"

void ui_handleXEvents(); // TODO: very x11 specific!

void ui_draw_status();
void ui_clear_status();
void ui_statusAdd( char c, Colour fg, bool bold );

void ui_draw();

void ui_main_draw();
void ui_main_newline();
void ui_main_print( const char * str, size_t len, Colour fg, Colour bg, bool bold );

void ui_chat_draw();
void ui_chat_newline();
void ui_chat_print( const char * str, size_t len, Colour fg, Colour bg, bool bold );

bool ui_urgent();

int ui_display_fd(); // TODO: very x11 specific!

void ui_init();
void ui_term();
