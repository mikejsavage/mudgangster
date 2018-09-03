#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <X11/Xlib.h>

#include "common.h"
#include <stdio.h>

void textbox_init( TextBox * tb ) {
	*tb = { };
	// TODO: this is kinda crap
	tb->text.lines = ( Line * ) calloc( sizeof( Line ), SCROLLBACK_SIZE );
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
	Line * line = &tb->text.lines[ ( tb->text.head + tb->text.num_lines ) % SCROLLBACK_SIZE ];
	size_t remaining = MAX_LINE_LENGTH - line->len;
	size_t n = min( strlen( str ), remaining );

	for( size_t i = 0; i < n; i++ ) {
		Glyph & glyph = line->glyphs[ line->len + i ];
		glyph.ch = str[ i ];
		glyph.fg = fg;
		glyph.bg = bg;
		glyph.bold = bold;
	};

	line->len += n;
}

void textbox_newline( TextBox * tb ) {
	if( tb->text.num_lines < SCROLLBACK_SIZE ) {
		tb->text.num_lines++;
		return;
	}

	tb->text.head++;
	Line * line = &tb->text.lines[ ( tb->text.head + tb->text.num_lines ) % SCROLLBACK_SIZE ];
	line->len = 0;
}

void textbox_draw( const TextBox * tb ) {
	if( tb->width == 0 || tb->height == 0 ) {
		return;
	}

	Pixmap doublebuf = XCreatePixmap( UI.display, UI.window, tb->width, tb->height, UI.depth );

	XSetForeground( UI.display, UI.gc, Style.bg );
	XFillRectangle( UI.display, doublebuf, UI.gc, 0, 0, tb->width, tb->height );

	size_t rows_drawn = 0;
	size_t tb_rows = tb->height / ( Style.font.height + SPACING );
	size_t tb_cols = tb->width / Style.font.width;

	for( size_t l = 0; l < tb->text.num_lines; l++ ) {
		const Line & line = tb->text.lines[ ( tb->text.head + tb->text.num_lines - tb->scroll_offset - l ) % SCROLLBACK_SIZE ];

		size_t rows = 1 + line.len / tb_cols;
		if( line.len > 0 && line.len % tb_cols == 0 )
			rows--;

		for( size_t i = 0; i < line.len; i++ ) {
			const Glyph & glyph = line.glyphs[ i ];

			size_t row = i / tb_cols;
			if( i > 0 && i % tb_cols == 0 )
				row--;

			int left = ( i % tb_cols ) * Style.font.width;
			// TODO: wrapping doesn't update top correctly
			int top = tb->height - ( 1 + rows_drawn + row ) * ( Style.font.height + SPACING );
			if( top < 0 )
				continue;

			// bg
			int top_spacing = SPACING / 2;
			int bot_spacing = SPACING - top_spacing;
			XSetForeground( UI.display, UI.gc,
				glyph.bg == SYSTEM ? Style.Colours.system : Style.colours[ 0 ][ glyph.bg ] );
			XFillRectangle( UI.display, doublebuf, UI.gc, left, top - top_spacing, Style.font.width, Style.font.height + bot_spacing );

			// fg
			XSetFont( UI.display, UI.gc, ( glyph.bold ? Style.fontBold : Style.font ).font->fid );
			XSetForeground( UI.display, UI.gc,
				glyph.fg == SYSTEM ? Style.Colours.system : Style.colours[ glyph.bold ][ glyph.fg ] );
			XDrawString( UI.display, doublebuf, UI.gc, left, top + Style.font.ascent + SPACING, &glyph.ch, 1 );
		}

		rows_drawn += rows;
		if( rows_drawn >= tb_rows )
			break;
	}

	XCopyArea( UI.display, doublebuf, UI.window, UI.gc, 0, 0, tb->width, tb->height, tb->x, tb->y );
	XFreePixmap( UI.display, doublebuf );
}

void TextBox::scroll( int offset ) {
	if( offset < 0 ) {
		scroll_offset -= min( size_t( -offset ), scroll_offset );
	}
	else {
		scroll_offset = min( scroll_offset + offset, text.num_lines );
	}

	textbox_draw( this );
}

void TextBox::page_down() {
	size_t rows = height / ( Style.font.height + SPACING );
	scroll( -int( rows ) - 2 );
}

void TextBox::page_up() {
	size_t rows = height / ( Style.font.height + SPACING );
	scroll( rows - 2 ); // TODO
}
