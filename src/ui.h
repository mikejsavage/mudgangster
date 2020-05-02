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

void ui_draw_status();
void ui_clear_status();
void ui_statusAdd( char c, Colour fg, bool bold );

void ui_main_newline();
void ui_main_print( const char * str, size_t len, Colour fg, Colour bg, bool bold );
void ui_chat_newline();
void ui_chat_print( const char * str, size_t len, Colour fg, Colour bg, bool bold );

void ui_fill_rect( int left, int top, int width, int height, Colour colour, bool bold );
void ui_draw_char( int left, int top, char c, Colour colour, bool bold, bool force_bold_font = false );

void ui_redraw_dirty();
void ui_redraw_everything();

void ui_update_layout();
void ui_resize( int width, int height );

void ui_scroll( int offset );
void ui_page_down();
void ui_page_up();

void ui_mouse_down( int x, int y );
void ui_mouse_move( int x, int y );
void ui_mouse_up( int x, int y );

void ui_get_font_size( int * fw, int * fh );

void ui_urgent();

void ui_init();
void ui_term();

void * platform_connect( const char ** err, const char * host, int port );
void platform_send( void * sock, const char * data, size_t len );
void platform_close( void * sock );

bool ui_set_font( const char * name, int size );
