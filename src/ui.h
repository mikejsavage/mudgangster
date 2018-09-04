#pragma once

#include "common.h"

// TODO: probably don't need most of these
enum Colour {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	SYSTEM,

	NUM_COLOURS,

	COLOUR_BG,
	COLOUR_STATUSBG,
	COLOUR_CURSOR,
};

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

void ui_fill_rect( int left, int top, int width, int height, Colour colour, bool bold );
void ui_draw_char( int left, int top, char c, Colour colour, bool bold, bool bold_font = false );
void ui_dirty( int left, int top, int right, int bottom ); // TODO: x/y + w/h?

void ui_get_font_size( int * fw, int * fh );

void ui_urgent();

int ui_display_fd(); // TODO: very x11 specific!

void ui_init();
void ui_term();
