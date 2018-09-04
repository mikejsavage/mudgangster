#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include "common.h"
#include "input.h"
#include "script.h"
#include "textbox.h"

struct {
	Display * display;
	int screen;

	Pixmap back_buffer;

	GC gc;
	Colormap colorMap;

	Window window;

	TextBox main_text;
	TextBox chat_text;

	int width;
	int height;
	int depth;
	int max_width, max_height;

	bool dirty;
	int dirty_left, dirty_top, dirty_right, dirty_bottom;

	bool has_focus;
} UI;

struct MudFont {
	int ascent;
	int descent;

	int height;
	int width;

	XFontStruct * font;
};

struct {
	ulong bg;
	ulong status_bg;
	ulong cursor;

	MudFont font;
	MudFont fontBold;

	union {
		struct {
			ulong black;
			ulong red;
			ulong green;
			ulong yellow;
			ulong blue;
			ulong magenta;
			ulong cyan;
			ulong white;

			ulong lblack;
			ulong lred;
			ulong lgreen;
			ulong lyellow;
			ulong lblue;
			ulong lmagenta;
			ulong lcyan;
			ulong lwhite;

			ulong system;
		} Colours;

		ulong colours[ 2 ][ 8 ];
	};
} Style;

static void set_fg( Colour colour, bool bold ) {
	ulong c;

	switch( colour ) {
		case SYSTEM:
			c = Style.Colours.system;
			break;

		case COLOUR_BG:
			c = Style.bg;
			break;

		case COLOUR_STATUSBG:
			c = Style.status_bg;
			break;

		case COLOUR_CURSOR:
			c = Style.cursor;
			break;

		default:
			c = Style.colours[ bold ][ colour ];
			break;
	}

	XSetForeground( UI.display, UI.gc, c );
}

void ui_fill_rect( int left, int top, int width, int height, Colour colour, bool bold ) {
	set_fg( colour, bold );
	XFillRectangle( UI.display, UI.back_buffer, UI.gc, left, top, width, height );
}

void ui_draw_char( int left, int top, char c, Colour colour, bool bold ) {
	int left_spacing = Style.font.width / 2;
	int right_spacing = Style.font.width - left_spacing;
	int line_height = Style.font.height + SPACING;
	int top_spacing = line_height / 2;
	int bot_spacing = line_height - top_spacing;

	// TODO: not the right char...
	// if( uint8_t( c ) == 155 ) { // fill
	// 	ui_fill_rect( left, top, Style.font.width, Style.font.height, colour, bold );
	// 	return;
	// }

	// TODO: this has a vertical seam. using textbox-space coordinates would help
	if( uint8_t( c ) == 176 ) { // light shade
		for( int y = 0; y < Style.font.height; y += 3 ) {
			for( int x = y % 6 == 0 ? 0 : 1; x < Style.font.width; x += 2 ) {
				ui_fill_rect( left + x, top + y, 1, 1, colour, bold );
			}
		}
		return;
	}

	// TODO: this has a horizontal seam but so does mm2k
	if( uint8_t( c ) == 177 ) { // medium shade
		for( int y = 0; y < Style.font.height; y += 2 ) {
			for( int x = y % 4 == 0 ? 1 : 0; x < Style.font.width; x += 2 ) {
				ui_fill_rect( left + x, top + y, 1, 1, colour, bold );
			}
		}
		return;
	}

	// TODO: this probably has a horizontal seam
	if( uint8_t( c ) == 178 ) { // heavy shade
		for( int y = 0; y < Style.font.height + SPACING; y++ ) {
			for( int x = y % 2 == 0 ? 1 : 0; x < Style.font.width; x += 2 ) {
				ui_fill_rect( left + x, top + y, 1, 1, colour, bold );
			}
		}
		return;
	}

	if( uint8_t( c ) == 179 ) { // vertical
		ui_fill_rect( left + left_spacing, top, 1, line_height, colour, bold );
		return;
		// set_fg( colour, bold );
		// const char asdf[] = "│";
		// Xutf8DrawString( UI.display, UI.back_buffer, ( bold ? Style.fontBold : Style.font ).font, UI.gc, left, top + Style.font.ascent + SPACING, asdf, sizeof( asdf ) - 1 );
	}

	if( uint8_t( c ) == 180 ) { // right stopper
		ui_fill_rect( left, top + top_spacing, left_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, line_height, colour, bold );
		return;
	}

	if( uint8_t( c ) == 186 ) { // double vertical
		ui_fill_rect( left + left_spacing - 1, top, 1, line_height, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top, 1, line_height, colour, bold );
		return;
	}

	if( uint8_t( c ) == 187 ) { // double top right
		ui_fill_rect( left, top + top_spacing - 1, right_spacing + 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top + top_spacing, 1, bot_spacing + 1, colour, bold );
		ui_fill_rect( left, top + top_spacing + 1, right_spacing - 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing - 1, top + top_spacing + 1, 1, bot_spacing - 1, colour, bold );
		return;
	}

	if( uint8_t( c ) == 188 ) { // double bottom right
		ui_fill_rect( left, top + top_spacing + 1, right_spacing + 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top, 1, top_spacing + 1, colour, bold );
		ui_fill_rect( left, top + top_spacing - 1, right_spacing - 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing - 1, top, 1, top_spacing - 1, colour, bold );
		return;
	}

	if( uint8_t( c ) == 191 ) { // top right
		ui_fill_rect( left, top + top_spacing, left_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top + top_spacing, 1, bot_spacing, colour, bold );
		return;
	}

	if( uint8_t( c ) == 192 ) { // bottom left
		ui_fill_rect( left + left_spacing, top + top_spacing, right_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, top_spacing, colour, bold );
		return;
	}

	if( uint8_t( c ) == 195 ) { // left stopper
		ui_fill_rect( left + left_spacing, top + top_spacing, right_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, line_height, colour, bold );
		return;
	}

	if( uint8_t( c ) == 196 ) { // horizontal
		ui_fill_rect( left, top + top_spacing, Style.font.width, 1, colour, bold );
		return;
	}

	if( uint8_t( c ) == 197 ) { // cross
		ui_fill_rect( left, top + top_spacing, Style.font.width, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, line_height, colour, bold );
		return;
	}

	if( uint8_t( c ) == 200 ) { // double bottom left
		ui_fill_rect( left + left_spacing - 1, top + top_spacing + 1, right_spacing + 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing - 1, top, 1, top_spacing + 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top + top_spacing - 1, right_spacing - 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top, 1, top_spacing - 1, colour, bold );
		return;
	}

	if( uint8_t( c ) == 201 ) { // double top left
		ui_fill_rect( left + left_spacing - 1, top + top_spacing - 1, right_spacing + 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing - 1, top + top_spacing - 1, 1, bot_spacing + 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top + top_spacing + 1, right_spacing - 1, 1, colour, bold );
		ui_fill_rect( left + left_spacing + 1, top + top_spacing + 1, 1, bot_spacing - 1, colour, bold );
		return;
	}

	if( uint8_t( c ) == 205 ) { // double horizontal
		ui_fill_rect( left, top + top_spacing - 1, Style.font.width, 1, colour, bold );
		ui_fill_rect( left, top + top_spacing + 1, Style.font.width, 1, colour, bold );
		return;
	}

	if( uint8_t( c ) == 217 ) { // bottom right
		ui_fill_rect( left, top + top_spacing, right_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top, 1, top_spacing, colour, bold );
		return;
	}

	if( uint8_t( c ) == 218 ) { // top left
		ui_fill_rect( left + left_spacing, top + top_spacing, right_spacing, 1, colour, bold );
		ui_fill_rect( left + left_spacing, top + top_spacing, 1, bot_spacing, colour, bold );
		return;
	}

	XSetFont( UI.display, UI.gc, ( bold ? Style.fontBold : Style.font ).font->fid );
	set_fg( colour, bold );
	XDrawString( UI.display, UI.back_buffer, UI.gc, left, top + Style.font.ascent + SPACING, &c, 1 );
}

void ui_dirty( int left, int top, int width, int height ) {
	XCopyArea( UI.display, UI.back_buffer, UI.window, UI.gc, left, top, width, height, left, top );

	// int right = left + width;
	// int bottom = top + height;
        //
	// printf( "make dirty %d %d %d %d\n", left, top, right, bottom );
	// if( !UI.dirty ) {
	// 	UI.dirty = true;
	// 	UI.dirty_left = left;
	// 	UI.dirty_top = top;
	// 	UI.dirty_right = right;
	// 	UI.dirty_bottom = bottom;
	// 	return;
	// }
        //
	// UI.dirty_left = min( UI.dirty_left, left );
	// UI.dirty_top = min( UI.dirty_top, top );
	// UI.dirty_right = max( UI.dirty_right, right );
	// UI.dirty_bottom = max( UI.dirty_bottom, bottom );
}

static Atom wmDeleteWindow;

typedef struct {
	char c;

	Colour fg;
	bool bold;
} StatusChar;

static StatusChar * statusContents = NULL;
static size_t statusCapacity = 256;
static size_t statusLen = 0;

void ui_clear_status() {
	statusLen = 0;
}

void ui_statusAdd( const char c, const Colour fg, const bool bold ) {
	if( ( statusLen + 1 ) * sizeof( StatusChar ) > statusCapacity ) {
		size_t newcapacity = statusCapacity * 2;
		StatusChar * newcontents = ( StatusChar * ) realloc( statusContents, newcapacity );

		if( !newcontents )
			return err( 1, "oom" );

		statusContents = newcontents;
		statusCapacity = newcapacity;
	}

	statusContents[ statusLen ] = ( StatusChar ) { c, fg, bold };
	statusLen++;
}

void ui_draw_status() {
	ui_fill_rect( 0, UI.height - PADDING * 4 - Style.font.height * 2, UI.width, Style.font.height + PADDING * 2, COLOUR_STATUSBG, false );

	for( size_t i = 0; i < statusLen; i++ ) {
		StatusChar sc = statusContents[ i ];

		int x = PADDING + i * Style.font.width;
		int y = UI.height - ( PADDING * 3 ) - Style.font.height * 2 - SPACING;
		ui_draw_char( x, y, sc.c, sc.fg, sc.bold );
	}

	ui_dirty( 0, UI.height - ( PADDING * 4 ) - ( Style.font.height * 2 ), UI.width, Style.font.height + ( PADDING * 2 ) );
}

void draw_input() {
	InputBuffer input = input_get_buffer();

	XSetFont( UI.display, UI.gc, Style.font.font->fid );

	int top = UI.height - PADDING - Style.font.height;
	ui_fill_rect( PADDING, top, UI.width - PADDING * 2, Style.font.height, COLOUR_BG, false );

	for( size_t i = 0; i < input.len; i++ )
		ui_draw_char( PADDING + i * Style.font.width, top - SPACING, input.buf[ i ], WHITE, false );

	ui_fill_rect( PADDING + input.cursor_pos * Style.font.width, top, Style.font.width, Style.font.height, COLOUR_CURSOR, false );

	if( input.cursor_pos < input.len ) {
		ui_draw_char( PADDING + input.cursor_pos * Style.font.width, top - SPACING, input.buf[ input.cursor_pos ], COLOUR_BG, false );
	}

	ui_dirty( PADDING, UI.height - ( PADDING + Style.font.height ), UI.width - PADDING * 2, Style.font.height );
}

void ui_draw() {
	ui_fill_rect( 0, 0, UI.width, UI.height, COLOUR_BG, false );

	draw_input();
	ui_draw_status();

	textbox_draw( &UI.chat_text );
	textbox_draw( &UI.main_text );

	int spacerY = ( 2 * PADDING ) + ( Style.font.height + SPACING ) * CHAT_ROWS;
	ui_fill_rect( 0, spacerY, UI.width, 1, COLOUR_STATUSBG, false );

	ui_dirty( 0, 0, UI.width, UI.height );
}

static void eventButtonPress( XEvent * event ) { }

static void eventButtonRelease( XEvent * event ) { }

static void eventMessage( XEvent * event ) {
	if( ( Atom ) event->xclient.data.l[ 0 ] == wmDeleteWindow ) {
		script_handleClose();
	}
}

static void eventResize( XEvent * event ) {
	int old_width = UI.width;
	int old_height = UI.height;

	UI.width = event->xconfigure.width;
	UI.height = event->xconfigure.height;

	if( UI.width == old_width && UI.height == old_height )
		return;

	int old_max_width = UI.max_width;
	int old_max_height = UI.max_height;
	UI.max_width = max( UI.max_width, UI.width );
	UI.max_height = max( UI.max_height, UI.height );

	if( UI.max_width != old_max_width || UI.max_height != old_max_height ) {
		if( old_width != -1 ) {
			XFreePixmap( UI.display, UI.back_buffer );
		}
		UI.back_buffer = XCreatePixmap( UI.display, UI.window, UI.max_width, UI.max_height, UI.depth );
	}

	textbox_set_pos( &UI.chat_text, PADDING, PADDING );
	textbox_set_size( &UI.chat_text, UI.width - ( 2 * PADDING ), ( Style.font.height + SPACING ) * CHAT_ROWS );

	textbox_set_pos( &UI.main_text, PADDING, ( PADDING * 2 ) + CHAT_ROWS * ( Style.font.height + SPACING ) + 1 );
	textbox_set_size( &UI.main_text, UI.width - ( 2 * PADDING ), UI.height
		- ( ( ( Style.font.height + SPACING ) * CHAT_ROWS ) + ( PADDING * 2 ) )
		- ( ( Style.font.height * 2 ) + ( PADDING * 5 ) ) - 1
	);
}

static void eventExpose( XEvent * event ) {
	ui_draw();
}

static void eventKeyPress( XEvent * event ) {
	#define ADD_MACRO( key, name ) \
		case key: \
			script_doMacro( name, sizeof( name ) - 1, shift, ctrl, alt ); \
			break

	XKeyEvent * keyEvent = &event->xkey;

	char keyBuffer[ 32 ];
	KeySym key;

	bool shift = keyEvent->state & ShiftMask;
	bool ctrl = keyEvent->state & ControlMask;
	bool alt = keyEvent->state & Mod1Mask;

	int len = XLookupString( keyEvent, keyBuffer, sizeof( keyBuffer ), &key, NULL );

	switch( key ) {
		case XK_Return:
			input_return();
			draw_input();
			break;

		case XK_BackSpace:
			input_backspace();
			draw_input();
			break;

		case XK_Delete:
			input_delete();
			draw_input();
			break;

		case XK_Page_Up:
			if( shift )
				textbox_scroll( &UI.main_text, 1 );
			else
				textbox_page_up( &UI.main_text );
			break;

		case XK_Page_Down:
			if( shift )
				textbox_scroll( &UI.main_text, -1 );
			else
				textbox_page_down( &UI.main_text );
			break;

		case XK_Up:
			input_up();
			draw_input();
			break;

		case XK_Down:
			input_down();
			draw_input();
			break;

		case XK_Left:
			input_left();
			draw_input();
			break;

		case XK_Right:
			input_right();
			draw_input();
			break;

		ADD_MACRO( XK_KP_1, "kp1" );
		ADD_MACRO( XK_KP_End, "kp1" );

		ADD_MACRO( XK_KP_2, "kp2" );
		ADD_MACRO( XK_KP_Down, "kp2" );

		ADD_MACRO( XK_KP_3, "kp3" );
		ADD_MACRO( XK_KP_Page_Down, "kp3" );

		ADD_MACRO( XK_KP_4, "kp4" );
		ADD_MACRO( XK_KP_Left, "kp4" );

		ADD_MACRO( XK_KP_5, "kp5" );
		ADD_MACRO( XK_KP_Begin, "kp5" );

		ADD_MACRO( XK_KP_6, "kp6" );
		ADD_MACRO( XK_KP_Right, "kp6" );

		ADD_MACRO( XK_KP_7, "kp7" );
		ADD_MACRO( XK_KP_Home, "kp7" );

		ADD_MACRO( XK_KP_8, "kp8" );
		ADD_MACRO( XK_KP_Up, "kp8" );

		ADD_MACRO( XK_KP_9, "kp9" );
		ADD_MACRO( XK_KP_Page_Up, "kp9" );

		ADD_MACRO( XK_KP_0, "kp0" );
		ADD_MACRO( XK_KP_Insert, "kp0" );

		ADD_MACRO( XK_KP_Multiply, "kp*" );
		ADD_MACRO( XK_KP_Divide, "kp/" );
		ADD_MACRO( XK_KP_Subtract, "kp-" );
		ADD_MACRO( XK_KP_Add, "kp+" );

		ADD_MACRO( XK_KP_Delete, "kp." );
		ADD_MACRO( XK_KP_Decimal, "kp." );

		ADD_MACRO( XK_F1, "f1" );
		ADD_MACRO( XK_F2, "f2" );
		ADD_MACRO( XK_F3, "f3" );
		ADD_MACRO( XK_F4, "f4" );
		ADD_MACRO( XK_F5, "f5" );
		ADD_MACRO( XK_F6, "f6" );
		ADD_MACRO( XK_F7, "f7" );
		ADD_MACRO( XK_F8, "f8" );
		ADD_MACRO( XK_F9, "f9" );
		ADD_MACRO( XK_F10, "f10" );
		ADD_MACRO( XK_F11, "f11" );
		ADD_MACRO( XK_F12, "f12" );

		default:
			if( ctrl || alt ) {
				script_doMacro( keyBuffer, len, shift, ctrl, alt );
			}
			else if( len > 0 ) {
				input_add( keyBuffer, len );
				draw_input();
			}

			break;
	}

	#undef ADD_MACRO
}

static void eventFocusOut( XEvent * event ) {
	UI.has_focus = false;
}

static void eventFocusIn( XEvent * event ) {
	UI.has_focus = true;

	XWMHints * hints = XGetWMHints( UI.display, UI.window );
	hints->flags &= ~XUrgencyHint;
	XSetWMHints( UI.display, UI.window, hints );
	XFree( hints );
}

void ui_handleXEvents() {
	void ( *event_handlers[ LASTEvent ] )( XEvent * ) = { };
	event_handlers[ ButtonPress ] = eventButtonPress;
	event_handlers[ ButtonRelease ] = eventButtonRelease;
	event_handlers[ ClientMessage ] = eventMessage;
	event_handlers[ ConfigureNotify ] = eventResize;
	event_handlers[ Expose ] = eventExpose;
	event_handlers[ KeyPress ] = eventKeyPress;
	event_handlers[ FocusOut ] = eventFocusOut;
	event_handlers[ FocusIn ] = eventFocusIn;

	const char * event_names[ LASTEvent ] = { };
	event_names[ ButtonPress ] = "ButtonPress";
	event_names[ ButtonRelease ] = "ButtonRelease";
	event_names[ ClientMessage ] = "ClientMessage";
	event_names[ ConfigureNotify ] = "ConfigureNotify";
	event_names[ Expose ] = "Expose";
	event_names[ KeyPress ] = "KeyPress";
	event_names[ FocusOut ] = "FocusOut";
	event_names[ FocusIn ] = "FocusIn";

	while( XPending( UI.display ) ) {
		XEvent event;
		XNextEvent( UI.display, &event );

		if( event_handlers[ event.type ] != NULL )
			event_handlers[ event.type ]( &event );
	}

	if( UI.dirty ) {
		XCopyArea( UI.display, UI.back_buffer, UI.window, UI.gc, UI.dirty_left, UI.dirty_top, UI.dirty_right - UI.dirty_left, UI.dirty_bottom - UI.dirty_top, UI.dirty_left, UI.dirty_top );
		UI.dirty = false;
	}
}

static MudFont loadFont( const char * fontStr ) {
	MudFont font;

	font.font = XLoadQueryFont( UI.display, fontStr );

	if( !font.font ) {
		printf( "could not load font %s\n", fontStr );

		exit( 1 );
	}

	font.ascent = font.font->ascent;
	font.descent = font.font->descent;

	font.width = font.font->max_bounds.rbearing - font.font->min_bounds.lbearing;
	font.height = font.ascent + font.descent;

	return font;
}

static ulong make_color( const char * hex ) {
	XColor color;
	XAllocNamedColor( UI.display, UI.colorMap, hex, &color, &color );
	return color.pixel;
}

static void initStyle() {
	Style.bg = make_color( "#1a1a1a" );
	Style.status_bg = make_color( "#333333" );
	Style.cursor = make_color( "#00ff00" );

	Style.Colours.black   = make_color( "#1a1a1a" );
	Style.Colours.red     = make_color( "#ca4433" );
	Style.Colours.green   = make_color( "#178a3a" );
	Style.Colours.yellow  = make_color( "#dc7c2a" );
	Style.Colours.blue    = make_color( "#415e87" );
	Style.Colours.magenta = make_color( "#5e468c" );
	Style.Colours.cyan    = make_color( "#35789b" );
	Style.Colours.white   = make_color( "#b6c2c4" );

	Style.Colours.lblack   = make_color( "#666666" );
	Style.Colours.lred     = make_color( "#ff2954" );
	Style.Colours.lgreen   = make_color( "#5dd030" );
	Style.Colours.lyellow  = make_color( "#fafc4f" );
	Style.Colours.lblue    = make_color( "#3581e1" );
	Style.Colours.lmagenta = make_color( "#875fff" );
	Style.Colours.lcyan    = make_color( "#29fbff" );
	Style.Colours.lwhite   = make_color( "#cedbde" );

	Style.Colours.system =   make_color( "#ffffff" );

	Style.font = loadFont( "-windows-dina-medium-r-normal--10-*-*-*-c-0-*-*" );
	Style.fontBold = loadFont( "-windows-dina-bold-r-normal--10-*-*-*-c-0-*-*" );
}

void ui_init() {
	UI = { };

	textbox_init( &UI.main_text, SCROLLBACK_SIZE );
	textbox_init( &UI.chat_text, CHAT_ROWS );
	UI.display = XOpenDisplay( NULL );
	UI.screen = XDefaultScreen( UI.display );
	UI.width = -1;
	UI.height = -1;

	Window root = XRootWindow( UI.display, UI.screen );
	UI.depth = XDefaultDepth( UI.display, UI.screen );
	Visual * visual = XDefaultVisual( UI.display, UI.screen );
	UI.colorMap = XDefaultColormap( UI.display, UI.screen );

	statusContents = ( StatusChar * ) malloc( statusCapacity * sizeof( StatusChar ) );

	if( statusContents == NULL ) {
		err( 1, "oom" );
	}

	initStyle();

	XSetWindowAttributes attr = { };
	attr.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | FocusChangeMask,
	attr.colormap = UI.colorMap,

	UI.window = XCreateWindow( UI.display, root, 0, 0, 800, 600, 0, UI.depth, InputOutput, visual, CWBackPixel | CWEventMask | CWColormap, &attr );
	UI.gc = XCreateGC( UI.display, UI.window, 0, NULL );

	XWMHints * hints = XAllocWMHints();
	XSetWMHints( UI.display, UI.window, hints );
	XFree( hints );

	Cursor cursor = XCreateFontCursor( UI.display, XC_xterm );
	XDefineCursor( UI.display, UI.window, cursor );

	XStoreName( UI.display, UI.window, "Mud Gangster" );
	XMapWindow( UI.display, UI.window );

	wmDeleteWindow = XInternAtom( UI.display, "WM_DELETE_WINDOW", false );
	XSetWMProtocols( UI.display, UI.window, &wmDeleteWindow, 1 );
}

void ui_main_draw() {
	textbox_draw( &UI.main_text );
}

void ui_main_newline() {
	textbox_newline( &UI.main_text );
}

void ui_main_print( const char * str, size_t len, Colour fg, Colour bg, bool bold ) {
	textbox_add( &UI.main_text, str, len, fg, bg, bold );
}

void ui_chat_draw() {
	textbox_draw( &UI.chat_text );
}

void ui_chat_newline() {
	textbox_newline( &UI.chat_text );
}

void ui_chat_print( const char * str, size_t len, Colour fg, Colour bg, bool bold ) {
	textbox_add( &UI.chat_text, str, len, fg, bg, bold );
}

void ui_urgent() {
	if( !UI.has_focus ) {
                XWMHints * hints = XGetWMHints( UI.display, UI.window );
                hints->flags |= XUrgencyHint;
                XSetWMHints( UI.display, UI.window, hints );
                XFree( hints );
        }
}

int ui_display_fd() {
	return ConnectionNumber( UI.display );
}

void ui_get_font_size( int * fw, int * fh ) {
	*fw = Style.font.width;
	*fh = Style.font.height;
}

void ui_term() {
	textbox_destroy( &UI.main_text );
	textbox_destroy( &UI.chat_text );
	free( statusContents );

	XFreeFont( UI.display, Style.font.font );
	XFreeFont( UI.display, Style.fontBold.font );

	XFreeGC( UI.display, UI.gc );
	XDestroyWindow( UI.display, UI.window );
	XCloseDisplay( UI.display );
}
