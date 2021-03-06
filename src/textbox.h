#pragma once

#include "ui.h"

constexpr size_t MAX_LINE_LENGTH = 2048;
constexpr size_t SCROLLBACK_SIZE = 1 << 14;

struct TextBox {
	struct Glyph {
		char ch;
		u8 style;
	};

	struct Line {
		Glyph glyphs[ MAX_LINE_LENGTH ];
		size_t len;
	};

	Span< Line > lines;
	size_t head;
	size_t num_lines;
	size_t max_lines;

	int x, y;
	int w, h;
	size_t scroll_offset;

	bool selecting;
	bool selecting_and_mouse_moved;
	bool scroll_down_after_selecting;
	int selection_start_col, selection_start_row;
	int selection_end_col, selection_end_row;

	bool dirty;
};

void textbox_init( TextBox * tb, size_t scrollback );

void textbox_add( TextBox * tb, const char * str, size_t len, Colour fg, Colour bg, bool bold );
void textbox_newline( TextBox * tb );

void textbox_scroll( TextBox * tb, int offset );
void textbox_page_down( TextBox * tb );
void textbox_page_up( TextBox * tb );

void textbox_mouse_down( TextBox * tb, int x, int y );
void textbox_mouse_move( TextBox * tb, int x, int y );
void textbox_mouse_up( TextBox * tb, int x, int y );

void textbox_set_pos( TextBox * tb, int x, int y );
void textbox_set_size( TextBox * tb, int w, int h );

void textbox_draw( TextBox * tb );

void textbox_destroy( TextBox * tb );
