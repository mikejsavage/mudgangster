#include <string.h>

#include "common.h"
#include "textbox.h"
#include "ui.h"

static uint8_t pack_style( Colour fg, Colour bg, bool bold ) {
	STATIC_ASSERT( NUM_COLOURS * NUM_COLOURS * 2 < UINT8_MAX );

	uint32_t style = 0;

	style = checked_cast< uint32_t >( fg );
	style = style * NUM_COLOURS + checked_cast< uint32_t >( bg );
	style = style * 2 + checked_cast< uint32_t >( bold );

	return checked_cast< uint8_t >( style );
}

static void unpack_style( uint8_t style, int * fg, int * bg, int * bold ) {
	*bold = style % 2;
	style /= 2;

	*bg = style % NUM_COLOURS;
	style /= NUM_COLOURS;

	*fg = style;
}

void textbox_init( TextBox * tb, size_t scrollback ) {
	// TODO: this is kinda crap
	tb->lines = ( TextBox::Line * ) calloc( sizeof( TextBox::Line ), scrollback );
	tb->num_lines = 1;
	tb->max_lines = scrollback;
}

void textbox_destroy( TextBox * tb ) {
	free( tb->lines );
}

void textbox_add( TextBox * tb, const char * str, size_t len, Colour fg, Colour bg, bool bold ) {
	TextBox::Line * line = &tb->lines[ ( tb->head + tb->num_lines ) % tb->max_lines ];
	size_t remaining = MAX_LINE_LENGTH - line->len;
	size_t n = min( strlen( str ), remaining );

	for( size_t i = 0; i < n; i++ ) {
		TextBox::Glyph & glyph = line->glyphs[ line->len + i ];
		glyph.ch = str[ i ];
		glyph.style = pack_style( fg, bg, bold );
	};

	line->len += n;
}

void textbox_newline( TextBox * tb ) {
	if( tb->num_lines < tb->max_lines ) {
		tb->num_lines++;
		return;
	}

	tb->head++;
	TextBox::Line * line = &tb->lines[ ( tb->head + tb->num_lines ) % tb->max_lines ];
	line->len = 0;
}

void textbox_scroll( TextBox * tb, int offset ) {
	if( offset < 0 ) {
		tb->scroll_offset -= min( size_t( -offset ), tb->scroll_offset );
	}
	else {
		tb->scroll_offset = min( tb->scroll_offset + offset, tb->num_lines - 1 );
	}

	textbox_draw( tb );
}

static size_t num_rows( size_t h ) {
	int fw, fh;
	ui_get_font_size( &fw, &fh );
	return h / ( fh + SPACING );
}

void textbox_page_down( TextBox * tb ) {
	textbox_scroll( tb, -int( num_rows( tb->h ) ) + 1 );
}

void textbox_page_up( TextBox * tb ) {
	textbox_scroll( tb, num_rows( tb->h ) - 1 );
}

void textbox_set_pos( TextBox * tb, int x, int y ) {
	tb->x = x;
	tb->y = y;
}

void textbox_set_size( TextBox * tb, int w, int h ) {
	tb->w = w;
	tb->h = h;
}

void textbox_draw( const TextBox * tb ) {
	if( tb->w == 0 || tb->h == 0 )
		return;

	ui_fill_rect( tb->x, tb->y, tb->w, tb->h, COLOUR_BG, false );

	/*
	 * lines refers to lines of text sent from the game
	 * rows refers to visual rows of text in the client, so when lines get
	 * wrapped they have more than one row
	 */

	int fw, fh;
	ui_get_font_size( &fw, &fh );

	size_t lines_drawn = 0;
	size_t rows_drawn = 0;
	size_t tb_rows = num_rows( tb->h );
	size_t tb_cols = tb->w / fw;

	while( rows_drawn < tb_rows && lines_drawn < tb->num_lines ) {
		const TextBox::Line & line = tb->lines[ ( tb->head + tb->num_lines - tb->scroll_offset - lines_drawn ) % tb->max_lines ];

		size_t line_rows = 1 + line.len / tb_cols;
		if( line.len > 0 && line.len % tb_cols == 0 )
			line_rows--;

		for( size_t i = 0; i < line.len; i++ ) {
			const TextBox::Glyph & glyph = line.glyphs[ i ];

			size_t row = i / tb_cols;

			int left = ( i % tb_cols ) * fw;
			int top = tb->h - ( rows_drawn + line_rows - row ) * ( fh + SPACING );
			if( top < 0 )
				continue;

			int fg, bg, bold;
			unpack_style( glyph.style, &fg, &bg, &bold );

			// bg
			// TODO: top/bottom spacing seems to be inconsistent here, try with large spacing
			int top_spacing = SPACING / 2;
			int bot_spacing = SPACING - top_spacing;
			ui_fill_rect( tb->x + left, tb->y + top - top_spacing, fw, fh + bot_spacing, Colour( bg ), false );

			// fg
			ui_draw_char( tb->x + left, tb->y + top, glyph.ch, Colour( fg ), bold );
		}

		lines_drawn++;
		rows_drawn += line_rows;
	}

	ui_dirty( tb->x, tb->y, tb->x + tb->w, tb->y + tb->h );
}
