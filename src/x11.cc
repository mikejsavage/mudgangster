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

	GC gc;
	Colormap colorMap;

	Window window;

	TextBufferView main_text_view;
	TextBufferView chat_text_view;

	int width;
	int height;
	int depth;

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
	ulong fg;

	XColor xBG;
	XColor xFG;

	ulong statusBG;
	ulong statusFG;

	ulong inputBG;
	ulong inputFG;
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

static void textview_draw( const TextBufferView * tv ) {
	if( tv->width == 0 || tv->height == 0 )
		return;

	Pixmap doublebuf = XCreatePixmap( UI.display, UI.window, tv->width, tv->height, UI.depth );

	XSetForeground( UI.display, UI.gc, Style.bg );
	XFillRectangle( UI.display, doublebuf, UI.gc, 0, 0, tv->width, tv->height );

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

	XCopyArea( UI.display, doublebuf, UI.window, UI.gc, 0, 0, tv->width, tv->height, tv->x, tv->y );
	XFreePixmap( UI.display, doublebuf );
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
	XSetForeground( UI.display, UI.gc, Style.statusBG );
	XFillRectangle( UI.display, UI.window, UI.gc, 0, UI.height - ( PADDING * 4 ) - ( Style.font.height * 2 ), UI.width, Style.font.height + ( PADDING * 2 ) );

	for( size_t i = 0; i < statusLen; i++ ) {
		StatusChar sc = statusContents[ i ];

		XSetFont( UI.display, UI.gc, ( sc.bold ? Style.fontBold : Style.font ).font->fid );
		XSetForeground( UI.display, UI.gc, Style.colours[ sc.bold ][ sc.fg ] );

		int x = PADDING + i * Style.font.width;
		int y = UI.height - ( PADDING * 3 ) - Style.font.height - Style.font.descent;
		XDrawString( UI.display, UI.window, UI.gc, x, y, &sc.c, 1 );
	}
}

void draw_input() {
	InputBuffer input = input_get_buffer();

	XSetFont( UI.display, UI.gc, Style.font.font->fid );

	XSetForeground( UI.display, UI.gc, Style.bg );
	XFillRectangle( UI.display, UI.window, UI.gc, PADDING, UI.height - ( PADDING + Style.font.height ), UI.width - 6, Style.font.height );

	XSetForeground( UI.display, UI.gc, Style.fg );
	XDrawString( UI.display, UI.window, UI.gc, PADDING, UI.height - ( PADDING + Style.font.descent ), input.buf, input.len );

	XSetForeground( UI.display, UI.gc, Style.cursor );
	XFillRectangle( UI.display, UI.window, UI.gc, PADDING + Style.font.width * input.cursor_pos, UI.height - ( PADDING + Style.font.height ), Style.font.width, Style.font.height );

	if( input.cursor_pos < input.len ) {
		XSetForeground( UI.display, UI.gc, Style.bg );
		XDrawString( UI.display, UI.window, UI.gc, PADDING + Style.font.width * input.cursor_pos, UI.height - ( PADDING + Style.font.descent ), input.buf + input.cursor_pos, 1 );
	}
}

void ui_draw() {
	XClearWindow( UI.display, UI.window );

	draw_input();
	ui_draw_status();

	textview_draw( &UI.chat_text_view );
	textview_draw( &UI.main_text_view );

	int spacerY = ( 2 * PADDING ) + ( Style.font.height + SPACING ) * CHAT_ROWS;
	XSetForeground( UI.display, UI.gc, Style.statusBG );
	XFillRectangle( UI.display, UI.window, UI.gc, 0, spacerY, UI.width, 1 );
}

static void eventButtonPress( XEvent * event ) { }

static void eventButtonRelease( XEvent * event ) { }

static void eventMessage( XEvent * event ) {
	if( ( Atom ) event->xclient.data.l[ 0 ] == wmDeleteWindow ) {
		script_handleClose();
	}
}

static void eventResize( XEvent * event ) {
	int newWidth = event->xconfigure.width;
	int newHeight = event->xconfigure.height;

	if( newWidth == UI.width && newHeight == UI.height )
		return;

	UI.width = newWidth;
	UI.height = newHeight;

	XSetForeground( UI.display, UI.gc, Style.bg );
	XFillRectangle( UI.display, UI.window, UI.gc, 0, 0, UI.width, UI.height );

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
			else {
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
	void ( *EventHandler[ LASTEvent ] )( XEvent * ) = { };
	EventHandler[ ButtonPress ] = eventButtonPress;
	EventHandler[ ButtonRelease ] = eventButtonRelease;
	EventHandler[ ClientMessage ] = eventMessage;
	EventHandler[ ConfigureNotify ] = eventResize;
	EventHandler[ Expose ] = eventExpose;
	EventHandler[ KeyPress ] = eventKeyPress;
	EventHandler[ FocusOut ] = eventFocusOut;
	EventHandler[ FocusIn ] = eventFocusIn;

	while( XPending( UI.display ) ) {
		XEvent event;
		XNextEvent( UI.display, &event );

		if( EventHandler[ event.type ] != NULL )
			EventHandler[ event.type ]( &event );
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

static void initStyle() {
	#define SETCOLOR( x, c ) \
		do { \
			XColor color; \
			XAllocNamedColor( UI.display, UI.colorMap, c, &color, &color ); \
			x = color.pixel; \
		} while( false )
	#define SETXCOLOR( x, y, c ) \
		do { \
			XColor color; \
			XAllocNamedColor( UI.display, UI.colorMap, c, &color, &color ); \
			x = color.pixel; \
			y = color; \
		} while( false )

	SETXCOLOR( Style.bg, Style.xBG, "#1a1a1a" );
	SETXCOLOR( Style.fg, Style.xFG, "#b6c2c4" );

	SETCOLOR( Style.cursor, "#00ff00" );

	SETCOLOR( Style.statusBG, "#333333" );
	SETCOLOR( Style.statusFG, "#ffffff" );

	SETCOLOR( Style.Colours.black,   "#1a1a1a" );
	SETCOLOR( Style.Colours.red,     "#ca4433" );
	SETCOLOR( Style.Colours.green,   "#178a3a" );
	SETCOLOR( Style.Colours.yellow,  "#dc7c2a" );
	SETCOLOR( Style.Colours.blue,    "#415e87" );
	SETCOLOR( Style.Colours.magenta, "#5e468c" );
	SETCOLOR( Style.Colours.cyan,    "#35789b" );
	SETCOLOR( Style.Colours.white,   "#b6c2c4" );

	SETCOLOR( Style.Colours.lblack,   "#666666" );
	SETCOLOR( Style.Colours.lred,     "#ff2954" );
	SETCOLOR( Style.Colours.lgreen,   "#5dd030" );
	SETCOLOR( Style.Colours.lyellow,  "#fafc4f" );
	SETCOLOR( Style.Colours.lblue,    "#3581e1" );
	SETCOLOR( Style.Colours.lmagenta, "#875fff" );
	SETCOLOR( Style.Colours.lcyan,    "#29fbff" );
	SETCOLOR( Style.Colours.lwhite,   "#cedbde" );

	SETCOLOR( Style.Colours.system,   "#ffffff" );

	#undef SETCOLOR
	#undef SETXCOLOR

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
	attr.background_pixel = Style.bg,
	attr.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | FocusChangeMask,
	attr.colormap = UI.colorMap,

	UI.window = XCreateWindow( UI.display, root, 0, 0, 800, 600, 0, UI.depth, InputOutput, visual, CWBackPixel | CWEventMask | CWColormap, &attr );
	UI.gc = XCreateGC( UI.display, UI.window, 0, NULL );

	XWMHints * hints = XAllocWMHints();
	XSetWMHints( UI.display, UI.window, hints );
	XFree( hints );

	Cursor cursor = XCreateFontCursor( UI.display, XC_xterm );
	XDefineCursor( UI.display, UI.window, cursor );
	XRecolorCursor( UI.display, cursor, &Style.xFG, &Style.xBG );

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
