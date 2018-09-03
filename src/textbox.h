#pragma once

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
	NONE,
};

constexpr size_t MAX_LINE_LENGTH = 2048;
constexpr size_t SCROLLBACK_SIZE = 1 << 16; // TODO

// TODO: pack these better
struct Glyph {
	char ch;
	Colour fg, bg;
	bool bold;
};

struct Line {
	Glyph glyphs[ MAX_LINE_LENGTH ];
	size_t len = 0;
};

struct Text {
	Line * lines;
	size_t head;
	size_t num_lines;
};

struct TextBox {
	Text text;

	int x;
	int y;
	int width;
	int height;

	size_t scroll_offset;

	void scroll( int offset );
	void page_up();
	void page_down();
};

void textbox_init( TextBox * tb );
void textbox_term( TextBox * tb );

void textbox_setpos( TextBox * tb, int x, int y );
void textbox_setsize( TextBox * tb, int width, int height );
void textbox_add( TextBox * tb, const char * str, unsigned int len, Colour fg, Colour bg, bool bold );
void textbox_newline( TextBox * tb );
void textbox_draw( const TextBox * tb );
