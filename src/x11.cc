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

struct TextBufferView {
	TextBuffer text;

	int x;
	int y;
	int width;
	int height;

	size_t scroll_offset;
};

struct {
	Display * display;
	int screen;

	Pixmap back_buffer;

	GC gc;
	Colormap colorMap;

	Window window;

	TextBufferView main_text_view;
	TextBufferView chat_text_view;

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

static void textview_draw( const TextBufferView * tv ) {
	if( tv->width == 0 || tv->height == 0 )
		return;

	ui_fill_rect( tv->x, tv->y, tv->width, tv->height, COLOUR_BG, false );

	/*
	 * lines refers to lines of text sent from the game
	 * rows refers to visual rows of text in the client, so when lines get
	 * wrapped they have more than one row
	 */
	size_t lines_drawn = 0;
	size_t rows_drawn = 0;
	size_t tv_rows = tv->height / ( Style.font.height + SPACING );
	size_t tv_cols = tv->width / Style.font.width;

	while( rows_drawn < tv_rows && lines_drawn < tv->text.num_lines ) {
		const TextBuffer::Line & line = tv->text.lines[ ( tv->text.head + tv->text.num_lines - tv->scroll_offset - lines_drawn ) % tv->text.max_lines ];

		size_t line_rows = 1 + line.len / tv_cols;
		if( line.len > 0 && line.len % tv_cols == 0 )
			line_rows--;

		for( size_t i = 0; i < line.len; i++ ) {
			const TextBuffer::Glyph & glyph = line.glyphs[ i ];

			size_t row = i / tv_cols;

			int left = ( i % tv_cols ) * Style.font.width;
			int top = tv->height - ( rows_drawn + line_rows - row ) * ( Style.font.height + SPACING );
			if( top < 0 )
				continue;

			int fg, bg, bold;
			unpack_style( glyph.style, &fg, &bg, &bold );

			// bg
			// TODO: top/bottom spacing seems to be inconsistent here, try with large spacing
			int top_spacing = SPACING / 2;
			int bot_spacing = SPACING - top_spacing;
			ui_fill_rect( tv->x + left, tv->y + top - top_spacing, Style.font.width, Style.font.height + bot_spacing, Colour( bg ), false );

			// fg
			ui_draw_char( tv->x + left, tv->y + top, glyph.ch, Colour( fg ), bold );
		}

		lines_drawn++;
		rows_drawn += line_rows;
	}

	ui_dirty( tv->x, tv->y, tv->x + tv->width, tv->y + tv->height );
}

static void textview_scroll( TextBufferView * tv, int offset ) {
	if( offset < 0 ) {
		tv->scroll_offset -= min( size_t( -offset ), tv->scroll_offset );
	}
	else {
		tv->scroll_offset = min( tv->scroll_offset + offset, tv->text.num_lines - 1 );
	}

	textview_draw( tv );
}

static void textview_page_down( TextBufferView * tv ) {
	size_t rows = tv->height / ( Style.font.height + SPACING );
	textview_scroll( tv, -int( rows ) + 1 );
}

static void textview_page_up( TextBufferView * tv ) {
	size_t rows = tv->height / ( Style.font.height + SPACING );
	textview_scroll( tv, rows - 1 );
}

static void textview_set_pos( TextBufferView * tv, int x, int y ) {
	tv->x = x;
	tv->y = y;
}

static void textview_set_size( TextBufferView * tv, int w, int h ) {
	tv->width = w;
	tv->height = h;
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

	textview_draw( &UI.chat_text_view );
	textview_draw( &UI.main_text_view );

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

	textview_set_pos( &UI.chat_text_view, PADDING, PADDING );
	textview_set_size( &UI.chat_text_view, UI.width - ( 2 * PADDING ), ( Style.font.height + SPACING ) * CHAT_ROWS );

	textview_set_pos( &UI.main_text_view, PADDING, ( PADDING * 2 ) + CHAT_ROWS * ( Style.font.height + SPACING ) + 1 );
	textview_set_size( &UI.main_text_view, UI.width - ( 2 * PADDING ), UI.height
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
				textview_scroll( &UI.main_text_view, 1 );
			else
				textview_page_up( &UI.main_text_view );
			break;

		case XK_Page_Down:
			if( shift )
				textview_scroll( &UI.main_text_view, -1 );
			else
				textview_page_down( &UI.main_text_view );
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

	text_init( &UI.main_text_view.text, SCROLLBACK_SIZE );
	text_init( &UI.chat_text_view.text, CHAT_ROWS );
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
	textview_draw( &UI.main_text_view );
}

void ui_main_newline() {
	text_newline( &UI.main_text_view.text );
}

void ui_main_print( const char * str, size_t len, Colour fg, Colour bg, bool bold ) {
	text_add( &UI.main_text_view.text, str, len, fg, bg, bold );
}

void ui_chat_draw() {
	textview_draw( &UI.chat_text_view );
}

void ui_chat_newline() {
	text_newline( &UI.chat_text_view.text );
}

void ui_chat_print( const char * str, size_t len, Colour fg, Colour bg, bool bold ) {
	text_add( &UI.chat_text_view.text, str, len, fg, bg, bold );
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

void ui_term() {
	text_destroy( &UI.main_text_view.text );
	text_destroy( &UI.chat_text_view.text );
	free( statusContents );

	XFreeFont( UI.display, Style.font.font );
	XFreeFont( UI.display, Style.fontBold.font );

	XFreeGC( UI.display, UI.gc );
	XDestroyWindow( UI.display, UI.window );
	XCloseDisplay( UI.display );
}
