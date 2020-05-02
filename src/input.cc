#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "array.h"
#include "input.h"
#include "script.h"
#include "ui.h"

#include "platform_ui.h"

static Span< char > history[ MAX_INPUT_HISTORY ] = { };
static size_t history_head = 0;
static size_t history_count = 0;
static size_t history_delta = 0;

static DynamicArray< char > input;

static size_t cursor_pos = 0;

static int left, top;
static int width, height;

static bool dirty = false;

void input_init() {
}

void input_term() {
}

void input_return() {
	if( input.size() > 0 ) {
		Span< const char > last_cmd = history_count == 0 ? Span< const char >() : history[ ( history_head + history_count - 1 ) % MAX_INPUT_HISTORY ];

		if( history_count == 0 || input.size() != last_cmd.n || memcmp( input.ptr(), last_cmd.ptr, last_cmd.num_bytes() ) != 0 ) {
			size_t pos = ( history_head + history_count ) % MAX_INPUT_HISTORY;

			if( history_count == MAX_INPUT_HISTORY ) {
				history_head++;
			}
			else {
				history_count++;
			}

			free( history[ pos ].ptr );
			history[ pos ] = alloc_span< char >( input.size() );
			memcpy( history[ pos ].ptr, input.ptr(), input.num_bytes() );
		}
	}

	script_handleInput( input.ptr(), input.size() );

	input.clear();
	history_delta = 0;
	cursor_pos = 0;
	dirty = true;
}

void input_backspace() {
	if( cursor_pos == 0 )
		return;
	cursor_pos--;
	input.remove( cursor_pos );
	dirty = true;
}

void input_delete() {
	if( input.remove( cursor_pos ) ) {
		dirty = true;
	}
}

static void input_from_history() {
	if( history_delta == 0 ) {
		input.clear();
	}
	else {
		Span< const char > cmd = history[ ( history_head + history_count - history_delta ) % MAX_INPUT_HISTORY ];
		input.from_span( cmd );
	}

	cursor_pos = input.size();
	dirty = true;
}

void input_up() {
	if( history_delta >= history_count )
		return;
	history_delta++;
	input_from_history();
}

void input_down() {
	if( history_delta == 0 )
		return;
	history_delta--;
	input_from_history();
}

void input_left() {
	if( cursor_pos > 0 ) {
		cursor_pos--;
		dirty = true;
	}
}

void input_right() {
	cursor_pos = min( cursor_pos + 1, input.size() );
	dirty = true;
}

void input_add( const char * buffer, int len ) {
	for( int i = 0; i < len; i++ ) {
		input.insert( buffer[ i ], cursor_pos );
		cursor_pos++;
	}

	dirty = true;
}

void input_set_pos( int x, int y ) {
	left = x;
	top = y;
}

void input_set_size( int w, int h ) {
	width = w;
	height = h;
}

bool input_is_dirty() {
	return dirty;
}

void input_draw() {
	int fw, fh;
	ui_get_font_size( &fw, &fh );

	ui_fill_rect( left, top, width, height, COLOUR_BG, false );

	size_t chars_that_fit = width / size_t( fw );
	size_t chars_to_draw = min( input.size(), chars_that_fit );

	for( size_t i = 0; i < chars_to_draw; i++ ) {
		ui_draw_char( PADDING + i * fw, top - SPACING, input[ i ], WHITE, false );
	}

	if( cursor_pos < chars_that_fit ) {
		ui_fill_rect( PADDING + cursor_pos * fw, top, fw, fh, COLOUR_CURSOR, false );

		if( cursor_pos < input.size() ) {
			ui_draw_char( PADDING + cursor_pos * fw, top - SPACING, input[ cursor_pos ], COLOUR_BG, false );
		}
	}

	platform_make_dirty( left, top, width, height );

	dirty = false;
}
