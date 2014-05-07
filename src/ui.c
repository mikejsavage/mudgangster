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

Atom wmDeleteWindow;

typedef struct {
	char c;

	Colour fg;
	bool bold;
} StatusChar;

static StatusChar* statusContents = NULL;
static size_t statusCapacity = 256;
static size_t statusLen = 0;

void ui_statusDraw()
{
	XSetForeground( UI.display, UI.gc, Style.statusBG );
	XFillRectangle( UI.display, UI.window, UI.gc, 0, UI.height - ( PADDING * 4 ) - ( Style.font.height * 2 ), UI.width, Style.font.height + ( PADDING * 2 ) );

	for( size_t i = 0; i < statusLen; i++ )
	{
		StatusChar sc = statusContents[ i ];

		XSetFont( UI.display, UI.gc, ( sc.bold ? Style.fontBold : Style.font ).font->fid );
		XSetForeground( UI.display, UI.gc, Style.colours[ sc.bold ][ sc.fg ] );

		int x = PADDING + i * Style.font.width;
		int y = UI.height - ( PADDING * 3 ) - Style.font.height - Style.font.descent;
		XDrawString( UI.display, UI.window, UI.gc, x, y, &sc.c, 1 );
	}
}

void ui_statusClear()
{
	statusLen = 0;
}

void ui_statusAdd( const char c, const Colour fg, const bool bold )
{
	if( ( statusLen + 1 ) * sizeof( StatusChar ) > statusCapacity )
	{
		size_t newcapacity = statusCapacity * 2;
		StatusChar* newcontents = realloc( statusContents, newcapacity );

		if( !newcontents )
		{
			return err( 1, "oom" );
		}

		statusContents = newcontents;
		statusCapacity = newcapacity;
	}

	statusContents[ statusLen ] = ( StatusChar ) { c, fg, bold };
	statusLen++;
}

void ui_draw()
{
	XClearWindow( UI.display, UI.window );

	input_draw();
	ui_statusDraw();

	textbox_draw( UI.textChat );
	textbox_draw( UI.textMain );

	int spacerY = ( 2 * PADDING ) + ( Style.font.height + SPACING ) * CHAT_ROWS;
	XSetForeground( UI.display, UI.gc, Style.statusBG );
	XFillRectangle( UI.display, UI.window, UI.gc, 0, spacerY, UI.width, 1 );
}

void eventButtonPress( XEvent* event )
{
	PRETEND_TO_USE( event );
}

void eventButtonRelease( XEvent* event )
{
	PRETEND_TO_USE( event );
}

void eventMessage( XEvent* event )
{
	if( ( Atom ) event->xclient.data.l[ 0 ] == wmDeleteWindow )
	{
		script_handleClose();
	}
}

void eventResize( XEvent* event )
{
	int newWidth = event->xconfigure.width;
	int newHeight = event->xconfigure.height;

	if( newWidth == UI.width && newHeight == UI.height )
	{
		return;
	}

	UI.width = newWidth;
	UI.height = newHeight;

	XSetForeground( UI.display, UI.gc, Style.bg );
	XFillRectangle( UI.display, UI.window, UI.gc, 0, 0, UI.width, UI.height );

	textbox_setpos( UI.textChat, PADDING, PADDING );
	textbox_setsize( UI.textChat, UI.width - ( 2 * PADDING ), ( Style.font.height + SPACING ) * CHAT_ROWS );

	textbox_setpos( UI.textMain, PADDING, ( PADDING * 2 ) + CHAT_ROWS * ( Style.font.height + SPACING ) + 1 );
	textbox_setsize( UI.textMain, UI.width - ( 2 * PADDING ), UI.height
		- ( ( ( Style.font.height + SPACING ) * CHAT_ROWS ) + ( PADDING * 2 ) )
		- ( ( Style.font.height * 2 ) + ( PADDING * 5 ) ) - 1
	);
}

void eventExpose( XEvent* event )
{
	PRETEND_TO_USE( event );

	ui_draw();
}

void eventKeyPress( XEvent* event )
{
	#define ADD_MACRO( key, name ) \
		case key: \
			script_doMacro( name, sizeof( name ) - 1, shift, ctrl, alt ); \
			break

	XKeyEvent* keyEvent = &event->xkey;

	char keyBuffer[ 32 ];
	KeySym key;

	bool shift = keyEvent->state & ShiftMask;
	bool ctrl = keyEvent->state & ControlMask;
	bool alt = keyEvent->state & Mod1Mask;

	int len = XLookupString( keyEvent, keyBuffer, sizeof( keyBuffer ), &key, NULL );

	switch( key )
	{
		case XK_Return:
		{
			input_send();

			break;
		}

		case XK_BackSpace:
			input_backspace();

			break;

		case XK_Delete:
			input_delete();

			break;

		case XK_Page_Up:
			if( UI.textMain->scrollDelta < UI.textMain->numLines )
			{
				int toScroll = shift ? 1 : ( UI.textMain->rows - 2 );

				UI.textMain->scrollDelta = MIN( UI.textMain->scrollDelta + toScroll, UI.textMain->numLines );

				textbox_draw( UI.textMain );
			}

			break;

		case XK_Page_Down:
			if( UI.textMain->scrollDelta > 0 )
			{
				int toScroll = shift ? 1 : ( UI.textMain->rows - 2 );

				UI.textMain->scrollDelta = MAX( UI.textMain->scrollDelta - toScroll, 0 );

				textbox_draw( UI.textMain );
			}

			break;

		case XK_Up:
			input_up();

			break;
		case XK_Down:
			input_down();

			break;

		case XK_Left:
			input_left();

			break;

		case XK_Right:
			input_right();

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
			if( ctrl || alt )
			{
				script_doMacro( keyBuffer, len, shift, ctrl, alt );
			}
			else
			{
				input_add( keyBuffer, len );
			}

			break;
	}

	#undef ADD_MACRO
}

void eventFocusOut( XEvent* event ) {
	PRETEND_TO_USE( event );

	UI.hasFocus = 0;
}

void eventFocusIn( XEvent* event )
{
	PRETEND_TO_USE( event );

	UI.hasFocus = 1;

	XWMHints* hints = XGetWMHints( UI.display, UI.window );
	hints->flags &= ~XUrgencyHint;
	XSetWMHints( UI.display, UI.window, hints );
	XFree( hints );
}

void ( *EventHandler[ LASTEvent ] ) ( XEvent* ) =
{
	[ ButtonPress ] = eventButtonPress,
	[ ButtonRelease ] = eventButtonRelease,
	[ ClientMessage ] = eventMessage,
	[ ConfigureNotify ] = eventResize,
	[ Expose ] = eventExpose,
	[ KeyPress ] = eventKeyPress,
	[ FocusOut ] = eventFocusOut,
	[ FocusIn ] = eventFocusIn,
};

void ui_handleXEvents()
{
	while( XPending( UI.display ) )
	{
		XEvent event;
		XNextEvent( UI.display, &event );

		if( EventHandler[ event.type ] )
		{
			EventHandler[ event.type ]( &event );
		}
	}
}

MudFont loadFont( char* fontStr )
{
	MudFont font;

	font.font = XLoadQueryFont( UI.display, fontStr );

	if( !font.font )
	{
		printf( "could not load font %s\n", fontStr );

		exit( 1 );
	}

	font.ascent = font.font->ascent;
	font.descent = font.font->descent;
	font.lbearing = font.font->min_bounds.lbearing;
	font.rbearing = font.font->max_bounds.rbearing;

	font.width = font.rbearing - font.lbearing;
	font.height = font.ascent + font.descent;

	return font;
}

void initStyle()
{
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

	Style.font = loadFont( "-windows-dina-medium-r-normal--12-120-75-75-c-0-microsoft-cp1252" );
	Style.fontBold = loadFont( "-windows-dina-bold-r-normal--12-120-75-75-c-0-microsoft-cp1252" );
}

void ui_init()
{
	UI.display = XOpenDisplay( NULL );
	UI.screen = XDefaultScreen( UI.display );
	UI.width = -1;
	UI.height = -1;

	Window root = XRootWindow( UI.display, UI.screen );
	UI.depth = XDefaultDepth( UI.display, UI.screen );
	Visual* visual = XDefaultVisual( UI.display, UI.screen );
	UI.colorMap = XDefaultColormap( UI.display, UI.screen );

	UI.textMain = textbox_new( OUTPUT_MAX_LINES );
	UI.textChat = textbox_new( OUTPUT_MAX_LINES );
	statusContents = malloc( statusCapacity * sizeof( StatusChar ) );

	if( statusContents == NULL ) {
		err( 1, "oom" );
	}

	initStyle();

	XSetWindowAttributes attr =
	{
		.background_pixel = Style.bg,
		.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | FocusChangeMask,
		.colormap = UI.colorMap,
	};

	UI.window = XCreateWindow( UI.display, root, 0, 0, 800, 600, 0, UI.depth, InputOutput, visual, CWBackPixel | CWEventMask | CWColormap, &attr );
	UI.gc = XCreateGC( UI.display, UI.window, 0, NULL );

	XWMHints* hints = XAllocWMHints();
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

void ui_end()
{
	textbox_free( UI.textMain );
	textbox_free( UI.textChat );
	free( statusContents );

	XFreeFont( UI.display, Style.font.font );
	XFreeFont( UI.display, Style.fontBold.font );

	XFreeGC( UI.display, UI.gc );
	XDestroyWindow( UI.display, UI.window );
	XCloseDisplay( UI.display );
}
