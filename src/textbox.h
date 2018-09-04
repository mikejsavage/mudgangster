#pragma once

#include "common.h"

constexpr size_t MAX_LINE_LENGTH = 2048;
constexpr size_t SCROLLBACK_SIZE = 1 << 16;

struct TextBuffer {
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

	// int x, y;
	// int w, h;
	// size_t scroll_offset;
	// bool dirty;
};

void text_init( TextBuffer * tb, size_t scrollback );

void text_add( TextBuffer * tb, const char * str, size_t len, Colour fg, Colour bg, bool bold );
void text_newline( TextBuffer * tb );
void unpack_style( uint8_t style, int * fg, int * bg, int * bold );

void text_destroy( TextBuffer * tb );
