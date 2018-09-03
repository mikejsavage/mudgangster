#pragma once

#include <stdint.h>

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
};

constexpr size_t MAX_LINE_LENGTH = 2048;
constexpr size_t SCROLLBACK_SIZE = 1 << 16; // TODO

struct Glyph {
	char ch;
	uint8_t style;
};

struct Line {
	Glyph glyphs[ MAX_LINE_LENGTH ];
	size_t len = 0;
};

struct Text {
	Line * lines;
	size_t head;
	size_t num_lines;
	size_t max_lines;
};

struct TextBox {
	Text text;

	int x;
	int y;
	int width;
	int height;

	size_t scroll_offset;
};

void textbox_init( TextBox * tb, size_t scrollback );
void textbox_term( TextBox * tb );

void textbox_setpos( TextBox * tb, int x, int y );
void textbox_setsize( TextBox * tb, int width, int height );
void textbox_add( TextBox * tb, const char * str, unsigned int len, Colour fg, Colour bg, bool bold );
void textbox_newline( TextBox * tb );
void textbox_draw( const TextBox * tb );

void textbox_scroll( TextBox * tb, int offset );
void textbox_page_up( TextBox * tb );
void textbox_page_down( TextBox * tb );
