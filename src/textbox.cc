#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <X11/Xlib.h>

#include "common.h"
#include <stdio.h>

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
	*tb = { };
	// TODO: this is kinda crap
	tb->text.lines = ( Line * ) calloc( sizeof( Line ), scrollback );
	tb->text.num_lines = 1;
	tb->text.max_lines = scrollback;
}

void textbox_term( TextBox * tb ) {
	free( tb->text.lines );
}

void textbox_setpos( TextBox * tb, int x, int y ) {
	tb->x = x;
	tb->y = y;
}

void textbox_setsize( TextBox * tb, int width, int height ) {
	tb->width = width;
	tb->height = height;
}

void textbox_add( TextBox * tb, const char * str, unsigned int len, Colour fg, Colour bg, bool bold ) {
	Line * line = &tb->text.lines[ ( tb->text.head + tb->text.num_lines ) % tb->text.max_lines ];
	size_t remaining = MAX_LINE_LENGTH - line->len;
	size_t n = min( strlen( str ), remaining );

	for( size_t i = 0; i < n; i++ ) {
		Glyph & glyph = line->glyphs[ line->len + i ];
		glyph.ch = str[ i ];
		glyph.style = pack_style( fg, bg, bold );
	};

	line->len += n;
}

void textbox_newline( TextBox * tb ) {
	if( tb->text.num_lines < tb->text.max_lines ) {
		tb->text.num_lines++;
		return;
	}

	tb->text.head++;
	Line * line = &tb->text.lines[ ( tb->text.head + tb->text.num_lines ) % tb->text.max_lines ];
	line->len = 0;
}

void textbox_draw( const TextBox * tb ) {
	if( tb->width == 0 || tb->height == 0 )
		return;

	Pixmap doublebuf = XCreatePixmap( UI.display, UI.window, tb->width, tb->height, UI.depth );

	XSetForeground( UI.display, UI.gc, Style.bg );
	XFillRectangle( UI.display, doublebuf, UI.gc, 0, 0, tb->width, tb->height );

	/*
	 * lines refers to lines of text sent from the game
	 * rows refers to visual rows of text in the client, so when lines get
	 * wrapped they have more than one row
	 */
	size_t lines_drawn = 0;
	size_t rows_drawn = 0;
	size_t tb_rows = tb->height / ( Style.font.height + SPACING );
	size_t tb_cols = tb->width / Style.font.width;

	while( rows_drawn < tb_rows && lines_drawn < tb->text.num_lines ) {
		const Line & line = tb->text.lines[ ( tb->text.head + tb->text.num_lines - tb->scroll_offset - lines_drawn ) % tb->text.max_lines ];

		size_t line_rows = 1 + line.len / tb_cols;
		if( line.len > 0 && line.len % tb_cols == 0 )
			line_rows--;

		for( size_t i = 0; i < line.len; i++ ) {
			const Glyph & glyph = line.glyphs[ i ];

			size_t row = i / tb_cols;

			int left = ( i % tb_cols ) * Style.font.width;
			int top = tb->height - ( rows_drawn + line_rows - row ) * ( Style.font.height + SPACING );
			if( top < 0 )
				continue;

			int fg, bg, bold;
			unpack_style( glyph.style, &fg, &bg, &bold );

			// bg
			int top_spacing = SPACING / 2;
			int bot_spacing = SPACING - top_spacing;
			XSetForeground( UI.display, UI.gc, bg == SYSTEM ? Style.Colours.system : Style.colours[ 0 ][ bg ] );
			XFillRectangle( UI.display, doublebuf, UI.gc, left, top - top_spacing, Style.font.width, Style.font.height + bot_spacing );

			// fg
			XSetFont( UI.display, UI.gc, ( bold ? Style.fontBold : Style.font ).font->fid );
			XSetForeground( UI.display, UI.gc, fg == SYSTEM ? Style.Colours.system : Style.colours[ bold ][ fg ] );
			XDrawString( UI.display, doublebuf, UI.gc, left, top + Style.font.ascent + SPACING, &glyph.ch, 1 );
		}

		lines_drawn++;
		rows_drawn += line_rows;
	}

	XCopyArea( UI.display, doublebuf, UI.window, UI.gc, 0, 0, tb->width, tb->height, tb->x, tb->y );
	XFreePixmap( UI.display, doublebuf );
}

void textbox_scroll( TextBox * tb, int offset ) {
	if( offset < 0 ) {
		tb->scroll_offset -= min( size_t( -offset ), tb->scroll_offset );
	}
	else {
		tb->scroll_offset = min( tb->scroll_offset + offset, tb->text.num_lines - 1 );
	}

	textbox_draw( tb );
}

void textbox_page_down( TextBox * tb ) {
	size_t rows = tb->height / ( Style.font.height + SPACING );
	textbox_scroll( tb, -int( rows ) + 1 );
}

void textbox_page_up( TextBox * tb ) {
	size_t rows = tb->height / ( Style.font.height + SPACING );
	textbox_scroll( tb, rows - 1 );
}
