#pragma once

#include "ui.h"

constexpr size_t MAX_LINE_LENGTH = 2048;
constexpr size_t SCROLLBACK_SIZE = 1 << 16;

struct TextBox {
	struct Glyph {
		char ch;
		uint8_t style;
	};

	struct Line {
		Glyph glyphs[ MAX_LINE_LENGTH ];
		size_t len = 0;
	};

	Line * lines;
	size_t head;
	size_t num_lines;
	size_t max_lines;

	int x, y;
	int w, h;
	size_t scroll_offset;

	bool selecting;
	int selection_start_col, selection_start_row;
	int selection_end_col, selection_end_row;
};

void textbox_init( TextBox * tb, size_t scrollback );

void textbox_add( TextBox * tb, const char * str, size_t len, Colour fg, Colour bg, bool bold );
void textbox_newline( TextBox * tb );

void textbox_scroll( TextBox * tb, int offset );
void textbox_page_down( TextBox * tb );
void textbox_page_up( TextBox * tb );

void textbox_set_pos( TextBox * tb, int x, int y );
void textbox_set_size( TextBox * tb, int w, int h );

void textbox_draw( const TextBox * tb );

void textbox_destroy( TextBox * tb );
