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
		if( tb->scroll_offset > 0 )
			tb->scroll_offset++;
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

static bool inside_selection( int col, int row, int start_col, int start_row, int end_col, int end_row ) {
	int min_col = min( start_col, end_col );
	int min_row = min( start_row, end_row );
	int max_col = max( start_col, end_col );
	int max_row = max( start_row, end_row );

	if( row < min_row || row > max_row )
		return false;

	if( start_row == end_row )
		return col >= min_col && col <= max_col;

	if( row > min_row && row < max_row )
		return true;

	if( start_row < end_row ) {
		if( row == start_row )
			return col <= start_col;
		if( row == end_row )
			return col >= end_col;
	}
	else {
		if( row == start_row )
			return col >= start_col;
		if( row == end_row )
			return col <= end_col;
	}

	return false;
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

	int top_spacing = SPACING / 2;
	int bot_spacing = SPACING - top_spacing;

	while( rows_drawn < tb_rows && lines_drawn + tb->scroll_offset < tb->num_lines ) {
		const TextBox::Line & line = tb->lines[ ( tb->head + tb->num_lines - tb->scroll_offset - lines_drawn ) % tb->max_lines ];

		size_t line_rows = 1 + line.len / tb_cols;
		if( line.len > 0 && line.len % tb_cols == 0 )
			line_rows--;

		for( size_t i = 0; i < line.len; i++ ) {
			const TextBox::Glyph & glyph = line.glyphs[ i ];

			size_t row = i / tb_cols;
			size_t col = i % tb_cols;

			int left = col * fw;
			int top = tb->h - ( rows_drawn + line_rows - row ) * ( fh + SPACING );
			if( top < 0 )
				continue;

			int fg, bg, bold;
			unpack_style( glyph.style, &fg, &bg, &bold );

			bool bold_fg = bold;
			bool bold_bg = false;
			if( tb->selecting ) {
				if( inside_selection( col, rows_drawn + line_rows - row - 1, tb->selection_start_col, tb->selection_start_row, tb->selection_end_col, tb->selection_end_row ) ) {
					swap( fg, bg );
					swap( bold_fg, bold_bg );
				}
			}

			// bg
			// TODO: top/bottom spacing seems to be inconsistent here, try with large spacing
			ui_fill_rect( tb->x + left, tb->y + top - top_spacing, fw, fh + bot_spacing, Colour( bg ), bold_bg );

			// fg
			ui_draw_char( tb->x + left, tb->y + top, glyph.ch, Colour( fg ), bold_fg, bold );
		}

		lines_drawn++;
		rows_drawn += line_rows;
	}

	ui_dirty( tb->x, tb->y, tb->w, tb->h );
}
