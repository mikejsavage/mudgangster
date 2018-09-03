#include <string.h>

#include "common.h"
#include "textbox.h"

static uint8_t pack_style( Colour fg, Colour bg, bool bold ) {
	STATIC_ASSERT( NUM_COLOURS * NUM_COLOURS * 2 < UINT8_MAX );

	uint32_t style = 0;

	style = checked_cast< uint32_t >( fg );
	style = style * NUM_COLOURS + checked_cast< uint32_t >( bg );
	style = style * 2 + checked_cast< uint32_t >( bold );

	return checked_cast< uint8_t >( style );
}

void unpack_style( uint8_t style, int * fg, int * bg, int * bold ) {
	*bold = style % 2;
	style /= 2;

	*bg = style % NUM_COLOURS;
	style /= NUM_COLOURS;

	*fg = style;
}

void text_init( TextBuffer * tb, size_t scrollback ) {
	// TODO: this is kinda crap
	tb->lines = ( TextBuffer::Line * ) calloc( sizeof( TextBuffer::Line ), scrollback );
	tb->num_lines = 1;
	tb->max_lines = scrollback;
}

void text_destroy( TextBuffer * tb ) {
	free( tb->lines );
}

void text_add( TextBuffer * tb, const char * str, size_t len, Colour fg, Colour bg, bool bold ) {
	TextBuffer::Line * line = &tb->lines[ ( tb->head + tb->num_lines ) % tb->max_lines ];
	size_t remaining = MAX_LINE_LENGTH - line->len;
	size_t n = min( strlen( str ), remaining );

	for( size_t i = 0; i < n; i++ ) {
		TextBuffer::Glyph & glyph = line->glyphs[ line->len + i ];
		glyph.ch = str[ i ];
		glyph.style = pack_style( fg, bg, bold );
	};

	line->len += n;
}

void text_newline( TextBuffer * tb ) {
	if( tb->num_lines < tb->max_lines ) {
		tb->num_lines++;
		return;
	}

	tb->head++;
	TextBuffer::Line * line = &tb->lines[ ( tb->head + tb->num_lines ) % tb->max_lines ];
	line->len = 0;
}
