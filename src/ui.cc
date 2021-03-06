#include "platform_ui.h"
#include "array.h"
#include "input.h"
#include "textbox.h"
#include "gitversion.h"

static TextBox main_text;
static TextBox chat_text;

static int window_width, window_height;

typedef struct {
	char c;

	Colour fg;
	bool bold;
} StatusChar;

static DynamicArray< StatusChar > status;
static bool status_dirty = false;

void ui_init() {
	ZoneScoped;

	textbox_init( &main_text, SCROLLBACK_SIZE );
	textbox_init( &chat_text, CHAT_ROWS );

	ui_main_print( "> Mud Gangster ", 0, SYSTEM, BLACK, false );
	ui_main_print( APP_VERSION, 0, SYSTEM, BLACK, true );
}

void ui_term() {
	textbox_destroy( &main_text );
	textbox_destroy( &chat_text );
}

void ui_fill_rect( int left, int top, int width, int height, Colour colour, bool bold ) {
	platform_fill_rect( left, top, width, height, colour, bold );
}

void ui_draw_char( int left, int top, char c, Colour colour, bool bold, bool force_bold_font ) {
	int fw, fh;
	ui_get_font_size( &fw, &fh );

	int left_spacing = fw / 2;
	int right_spacing = fw - left_spacing;
	int line_height = fh + SPACING;
	int top_spacing = line_height / 2;
	int bot_spacing = line_height - top_spacing;

	// TODO: not the right char...
	// if( u8( c ) == 155 ) { // fill
	// 	ui_fill_rect( left, top, fw, fh, colour, bold );
	// 	return;
	// }

	// TODO: this has a vertical seam. using textbox-space coordinates would help
	if( u8( c ) == 176 ) { // light shade
		for( int y = 0; y < fh; y += 3 ) {
			for( int x = y % 6 == 0 ? 0 : 1; x < fw; x += 2 ) {
				ui_fill_rect( left + x, top + y, 1, 1, colour, bold );
			}
		}
		return;
	}

	// TODO: this has a horizontal seam but so does mm2k
	if( u8( c ) == 177 ) { // medium shade
		for( int y = 0; y < fh; y += 2 ) {
			for( int x = y % 4 == 0 ? 1 : 0; x < fw; x += 2 ) {
				ui_fill_rect( left + x, top + y, 1, 1, colour, bold );
			}
		}
		return;
	}

	// TODO: this probably has a horizontal seam
	if( u8( c ) == 178 ) { // heavy shade
		for( int y = 0; y < fh + SPACING; y++ ) {
			for( int x = y % 2 == 0 ? 1 : 0; x < fw; x += 2 ) {
				ui_fill_rect( left + x, top + y, 1, 1, colour, bold );
			}
		}
		return;
	}

	if( u8( c ) == 179 ) { // vertical
		ui_fill_rect( left + left_spacing, top, 1, line_height, colour, bold );
		return;
		// set_fg( colour, bold );
		// const char asdf[] = "│";
		// Xutf8DrawString( UI.display, UI.back_buffer, ( bold ? Style.fontBold : Style.font ).font, UI.gc, left, top + Style.font.ascent + SPACING, asdf, sizeof( asdf ) - 1 );
	}

	if( u8( c ) == 180 ) { // right stopper
		ui_fill_rect( left, top + top_spacing, left_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, line_height, colour, bold );
		return;
	}

	if( u8( c ) == 186 ) { // double vertical
		ui_fill_rect( left + left_spacing - 1, top, 1, line_height, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top, 1, line_height, colour, bold );
		return;
	}

	if( u8( c ) == 187 ) { // double top right
		ui_fill_rect( left, top + top_spacing - 1, right_spacing + 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top + top_spacing - 1, 1, bot_spacing + 1, colour, bold );
		ui_fill_rect( left, top + top_spacing + 1, right_spacing - 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing - 1, top + top_spacing + 1, 1, bot_spacing - 1, colour, bold );
		return;
	}

	if( u8( c ) == 188 ) { // double bottom right
		ui_fill_rect( left, top + top_spacing + 1, right_spacing + 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top, 1, top_spacing + 1, colour, bold );
		ui_fill_rect( left, top + top_spacing - 1, right_spacing - 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing - 1, top, 1, top_spacing - 1, colour, bold );
		return;
	}

	if( u8( c ) == 191 ) { // top right
		ui_fill_rect( left, top + top_spacing, left_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top + top_spacing, 1, bot_spacing, colour, bold );
		return;
	}

	if( u8( c ) == 192 ) { // bottom left
		ui_fill_rect( left + left_spacing, top + top_spacing, right_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, top_spacing, colour, bold );
		return;
	}

	if( u8( c ) == 193 ) { // bottom stopper
		ui_fill_rect( left + left_spacing, top, 1, top_spacing, colour, bold );
		ui_fill_rect( left, top + top_spacing, fw, 1, colour, bold );
		return;
	}

	if( u8( c ) == 194 ) { // top stopper
		ui_fill_rect( left + left_spacing, top + top_spacing, 1, bot_spacing, colour, bold );
		ui_fill_rect( left, top + top_spacing, fw, 1, colour, bold );
		return;
	}

	if( u8( c ) == 195 ) { // left stopper
		ui_fill_rect( left + left_spacing, top + top_spacing, right_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, line_height, colour, bold );
		return;
	}

	if( u8( c ) == 196 ) { // horizontal
		ui_fill_rect( left, top + top_spacing, fw, 1, colour, bold );
		return;
	}

	if( u8( c ) == 197 ) { // cross
		ui_fill_rect( left, top + top_spacing, fw, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, line_height, colour, bold );
		return;
	}

	if( u8( c ) == 200 ) { // double bottom left
		ui_fill_rect( left + left_spacing - 1, top + top_spacing + 1, right_spacing + 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing - 1, top, 1, top_spacing + 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top + top_spacing - 1, right_spacing - 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top, 1, top_spacing - 1, colour, bold );
		return;
	}

	if( u8( c ) == 201 ) { // double top left
		ui_fill_rect( left + left_spacing - 1, top + top_spacing - 1, right_spacing + 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing - 1, top + top_spacing - 1, 1, bot_spacing + 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top + top_spacing + 1, right_spacing - 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top + top_spacing + 1, 1, bot_spacing - 1, colour, bold );
		return;
	}

	if( u8( c ) == 205 ) { // double horizontal
		ui_fill_rect( left, top + top_spacing - 1, fw, 1, colour, bold );
		ui_fill_rect( left, top + top_spacing + 1, fw, 1, colour, bold );
		return;
	}

	if( u8( c ) == 217 ) { // bottom right
		ui_fill_rect( left, top + top_spacing, right_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, top_spacing, colour, bold );
		return;
	}

	if( u8( c ) == 218 ) { // top left
		ui_fill_rect( left + left_spacing, top + top_spacing, right_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top + top_spacing, 1, bot_spacing, colour, bold );
		return;
	}

	platform_draw_char( left, top, c, colour, bold, force_bold_font );
}

void ui_clear_status() {
	status.clear();
	status_dirty = true;
}

void ui_statusAdd( char c, Colour fg, bool bold ) {
	status.add( { c, fg, bold } );
	status_dirty = true;
}

void ui_draw_status() {
	int fw, fh;
	ui_get_font_size( &fw, &fh );

	ui_fill_rect( 0, window_height - PADDING * 4 - fh * 2, window_width, fh + PADDING * 2, COLOUR_STATUSBG, false );

	for( size_t i = 0; i < status.size(); i++ ) {
		StatusChar sc = status[ i ];

		int x = PADDING + i * fw;
		int y = window_height - ( PADDING * 3 ) - fh * 2 - SPACING;
		ui_draw_char( x, y, sc.c, sc.fg, sc.bold );
	}

	platform_make_dirty( 0, window_height - PADDING * 4 - fh * 2, window_width, fh + PADDING * 2 );

	status_dirty = false;
}

void ui_redraw_dirty() {
	if( main_text.dirty )
		textbox_draw( &main_text );
	if( chat_text.dirty )
		textbox_draw( &chat_text );
	if( input_is_dirty() )
		input_draw();
	if( status_dirty )
		ui_draw_status();
}

void ui_redraw_everything() {
	int fw, fh;
	ui_get_font_size( &fw, &fh );

	ui_fill_rect( 0, 0, window_width, window_height, COLOUR_BG, false );

	input_draw();
	ui_draw_status();

	textbox_draw( &main_text );
	textbox_draw( &chat_text );

	int spacerY = ( 2 * PADDING ) + ( fh + SPACING ) * CHAT_ROWS;
	ui_fill_rect( 0, spacerY, window_width, 1, COLOUR_STATUSBG, false );
}

void ui_main_newline() {
	textbox_newline( &main_text );
}

void ui_main_print( const char * str, size_t len, Colour fg, Colour bg, bool bold ) {
	textbox_add( &main_text, str, len, fg, bg, bold );
}

void ui_chat_newline() {
	textbox_newline( &chat_text );
}

void ui_chat_print( const char * str, size_t len, Colour fg, Colour bg, bool bold ) {
	textbox_add( &chat_text, str, len, fg, bg, bold );
}

void ui_update_layout() {
	int fw, fh;
	ui_get_font_size( &fw, &fh );

	textbox_set_pos( &chat_text, PADDING, PADDING );
	textbox_set_size( &chat_text, window_width - ( 2 * PADDING ), ( fh + SPACING ) * CHAT_ROWS );

	textbox_set_pos( &main_text, PADDING, ( PADDING * 2 ) + CHAT_ROWS * ( fh + SPACING ) + 1 );
	textbox_set_size( &main_text, window_width - ( 2 * PADDING ), window_height
		- ( ( ( fh + SPACING ) * CHAT_ROWS ) + ( PADDING * 2 ) )
		- ( ( fh * 2 ) + ( PADDING * 5 ) ) - 1
	);

	input_set_pos( PADDING, window_height - PADDING - fh );
	input_set_size( window_width - PADDING * 2, fh );
}

void ui_resize( int width, int height ) {
	int old_width = window_width;
	int old_height = window_height;

	window_width = width;
	window_height = height;

	if( window_width == old_width && window_height == old_height )
		return;

	ui_update_layout();
}

void ui_scroll( int offset ) {
	textbox_scroll( &main_text, offset );
}

void ui_page_down() {
	textbox_page_down( &main_text );
}

void ui_page_up() {
	textbox_page_up( &main_text );
}

void ui_mouse_down( int x, int y ) {
	textbox_mouse_down( &main_text, x, y );
	textbox_mouse_down( &chat_text, x, y );
}

void ui_mouse_up( int x, int y ) {
	textbox_mouse_up( &main_text, x, y );
	textbox_mouse_up( &chat_text, x, y );
}

void ui_mouse_move( int x, int y ) {
	textbox_mouse_move( &main_text, x, y );
	textbox_mouse_move( &chat_text, x, y );
}
