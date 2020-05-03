#include <string.h>

#include "common.h"
#include "textbox.h"
#include "ui.h"
#include "platform.h"
#include "platform_ui.h"

#if PLATFORM_WINDOWS
#define NEWLINE_STRING "\r\n"
#else
#define NEWLINE_STRING "\n"
#endif

static u8 pack_style( Colour fg, Colour bg, bool bold ) {
	STATIC_ASSERT( NUM_COLOURS * NUM_COLOURS * 2 < UINT8_MAX );

	u32 style = 0;
	style = checked_cast< u32 >( fg );
	style = style * NUM_COLOURS + checked_cast< u32 >( bg );
	style = style * 2 + checked_cast< u32 >( bold );

	return checked_cast< u8 >( style );
}

static void unpack_style( u8 style, int * fg, int * bg, int * bold ) {
	*bold = style % 2;
	style /= 2;

	*bg = style % NUM_COLOURS;
	style /= NUM_COLOURS;

	*fg = style;
}

void textbox_init( TextBox * tb, size_t scrollback ) {
	ZoneScoped;

	// TODO: this is kinda crap
	*tb = { };
	tb->lines = alloc_span< TextBox::Line >( scrollback );
	memset( tb->lines.ptr, 0, tb->lines.num_bytes() );
	tb->num_lines = 1;
	tb->max_lines = scrollback;
}

void textbox_destroy( TextBox * tb ) {
	free( tb->lines.ptr );
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
	tb->dirty = true;
}

void textbox_newline( TextBox * tb ) {
	bool freeze = tb->scroll_offset > 0 || tb->selecting;

	if( tb->num_lines < tb->max_lines ) {
		tb->num_lines++;
		if( freeze )
			tb->scroll_offset++;
		else
			tb->dirty = true;
		return;
	}

	tb->head++;
	if( freeze )
		tb->scroll_offset = min( tb->scroll_offset + 1, tb->num_lines - 1 );
	tb->dirty = true;

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

	tb->dirty = true;
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

void textbox_mouse_down( TextBox * tb, int window_x, int window_y ) {
	int x = window_x - tb->x;
	int y = window_y - tb->y;

	if( x < 0 || y < 0 || x >= tb->w || y >= tb->h )
		return;

	int fw, fh;
	ui_get_font_size( &fw, &fh );

	int row = ( tb->h - y ) / ( fh + SPACING );
	int col = x / fw;

	tb->selecting = true;
	tb->selecting_and_mouse_moved = false;
	tb->scroll_down_after_selecting = tb->scroll_offset == 0;
	tb->selection_start_col = col;
	tb->selection_start_row = row;
	tb->selection_end_col = col;
	tb->selection_end_row = row;
	tb->dirty = true;
}

void textbox_mouse_move( TextBox * tb, int window_x, int window_y ) {
	if( !tb->selecting )
		return;

	int x = window_x - tb->x;
	int y = window_y - tb->y;

	int fw, fh;
	ui_get_font_size( &fw, &fh );

	int row = ( tb->h - y ) / ( fh + SPACING );
	int col = x / fw;

	if( col != tb->selection_end_col || row != tb->selection_end_row ) {
		tb->selection_end_col = col;
		tb->selection_end_row = row;
		tb->dirty = true;
	}
	else if( !tb->selecting_and_mouse_moved ) {
		tb->dirty = true;
	}

	tb->selecting_and_mouse_moved = true;
}

void textbox_mouse_up( TextBox * tb, int window_x, int window_y ) {
	if( !tb->selecting || !tb->selecting_and_mouse_moved ) {
		tb->selecting = false;
		return;
	}

	int fw, fh;
	ui_get_font_size( &fw, &fh );

	size_t tb_cols = tb->w / fw;

	// convert mouse start/end points to ordered start/end points
	int start_row = tb->selection_start_row;
	int end_row = tb->selection_end_row;
	int start_col = tb->selection_start_col;
	int end_col = tb->selection_end_col;
	if( tb->selection_start_row == tb->selection_end_row ) {
		start_col = min( tb->selection_start_col, tb->selection_end_col );
		end_col = max( tb->selection_start_col, tb->selection_end_col );
	}
	else if( tb->selection_start_row < tb->selection_end_row ) {
		swap( start_row, end_row );
		swap( start_col, end_col );
	}

	// find what the start/end lines/offsets are
	// TODO: this is incorrect when wrapping...
	int end_line = 0;
	int rows = 0;

	while( rows < end_row && end_line < tb->num_lines ) {
		const TextBox::Line & line = tb->lines[ ( tb->head + tb->num_lines - tb->scroll_offset - end_line ) % tb->max_lines ];
		int line_rows = 1 + line.len / tb_cols;
		if( line.len > 0 && line.len % tb_cols == 0 )
			line_rows--;
		rows += line_rows;
		end_line++;
	}

	if( end_line == tb->num_lines )
		return;

	size_t end_line_offset = ( rows - end_row ) * tb_cols + end_col + 1;
	int start_line = end_line;

	while( rows < start_row && start_line < tb->num_lines ) {
		const TextBox::Line & line = tb->lines[ ( tb->head + tb->num_lines - tb->scroll_offset - start_line ) % tb->max_lines ];
		int line_rows = 1 + line.len / tb_cols;
		if( line.len > 0 && line.len % tb_cols == 0 )
			line_rows--;
		rows += line_rows;
		start_line++;
	}

	size_t start_line_offset = ( rows - start_row ) * tb_cols + start_col;
	if( start_line == tb->num_lines ) {
		start_line--;
		start_line_offset = 0;
	}

	// first pass to get the length of the selected string
	size_t selected_length = 1; // include space for \0
	for( int i = start_line; i >= end_line; i-- ) {
		const TextBox::Line & line = tb->lines[ ( tb->head + tb->num_lines - tb->scroll_offset - i ) % tb->max_lines ];
		size_t start_offset = i == start_line ? start_line_offset : 0;
		size_t end_offset = i == end_line ? end_line_offset : line.len;
		// TODO: iterate over glyphs to see when ansi codes need inserting
		if( start_offset <= line.len ) {
			selected_length += min( line.len, end_offset ) - start_offset;
		}
		if( i != end_line ) {
			selected_length += sizeof( NEWLINE_STRING ) - 1;
		}
	}

	char * selected = alloc_many< char >( selected_length );
	selected[ selected_length - 1 ] = '\0';

	// second pass to copy the selection out
	size_t n = 0;
	for( int i = start_line; i >= end_line; i-- ) {
		const TextBox::Line & line = tb->lines[ ( tb->head + tb->num_lines - tb->scroll_offset - i ) % tb->max_lines ];
		size_t start_offset = i == start_line ? start_line_offset : 0;
		size_t end_offset = i == end_line ? end_line_offset : line.len;
		if( start_offset <= line.len ) {
			size_t len = min( line.len, end_offset ) - start_offset;
			// TODO: insert ansi codes when style changes
			for( size_t j = 0; j < len; j++ ) {
				selected[ n ] = line.glyphs[ j + start_offset ].ch;
				n++;
			}
		}
		if( i != end_line ) {
			memcpy( selected + n, NEWLINE_STRING, sizeof( NEWLINE_STRING ) - 1 );
			n += sizeof( NEWLINE_STRING ) - 1;
		}
	}

	platform_set_clipboard( selected, selected_length );
	free( selected );

	tb->selecting = false;
	tb->dirty = true;

	if( tb->scroll_down_after_selecting )
		tb->scroll_offset = 0;
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
	int min_row = min( start_row, end_row );
	int max_row = max( start_row, end_row );

	if( row < min_row || row > max_row )
		return false;

	if( start_row == end_row ) {
		int min_col = min( start_col, end_col );
		int max_col = max( start_col, end_col );
		return col >= min_col && col <= max_col;
	}

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

void textbox_draw( TextBox * tb ) {
	if( tb->w <= 0 || tb->h <= 0 )
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
			if( tb->selecting && tb->selecting_and_mouse_moved ) {
				if( inside_selection( col, rows_drawn + line_rows - row - 1, tb->selection_start_col, tb->selection_start_row, tb->selection_end_col, tb->selection_end_row ) ) {
					swap( fg, bg );
					swap( bold_fg, bold_bg );
				}
			}

			// bg
			// TODO: top/bottom spacing seems to be inconsistent here, try with large spacing
			if( bg != BLACK ) {
				ui_fill_rect( tb->x + left, tb->y + top - top_spacing, fw, fh + bot_spacing, Colour( bg ), bold_bg );
			}

			// fg
			ui_draw_char( tb->x + left, tb->y + top, glyph.ch, Colour( fg ), bold_fg, bold );
		}

		lines_drawn++;
		rows_drawn += line_rows;
	}

	platform_make_dirty( tb->x, tb->y, tb->w, tb->h );

	tb->dirty = false;
}
