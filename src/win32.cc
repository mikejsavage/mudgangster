#include <windows.h>
#include <windowsx.h>
#include <Winsock2.h>
#include <stdio.h>

#include "common.h"
#include "input.h"
#include "script.h"
#include "textbox.h"

#include "platform_network.h"

#define WINDOW_CLASSNAME "MudGangsterClass"

struct {
	HWND hwnd;
	HDC hdc;

	TextBox main_text;
	TextBox chat_text;

	int width, height;

	bool has_focus;
} UI;

struct Socket {
	TCPSocket sock;
	bool in_use;
};

static Socket sockets[ 128 ];

void * platform_connect( const char ** err, const char * host, int port ) {
	size_t idx;
	{
		bool ok = false;
		for( size_t i = 0; i < ARRAY_COUNT( sockets ); i++ ) {
			if( !sockets[ i ].in_use ) {
				idx = i;
				ok = true;
				break;
			}
		}

		if( !ok ) {
			*err = "too many connections";
			return NULL;
		}
	}

	NetAddress addr;
	{
		bool ok = dns_first( host, &addr );
		if( !ok ) {
			*err = "couldn't resolve hostname"; // TODO: error from dns_first
			return NULL;
		}
	}
	addr.port = checked_cast< u16 >( port );

	TCPSocket sock;
	bool ok = net_new_tcp( &sock, addr, NET_BLOCKING );
	if( !ok ) {
		*err = "net_new_tcp";
		return NULL;
	}

	sockets[ idx ].sock = sock;
	sockets[ idx ].in_use = true;

	WSAAsyncSelect( sock.fd, UI.hwnd, 12345, FD_READ | FD_CLOSE );

	return &sockets[ idx ];
}

void platform_send( void * vsock, const char * data, size_t len ) {
	Socket * sock = ( Socket * ) vsock;
	net_send( sock->sock, data, len );
}

void platform_close( void * vsock ) {
	Socket * sock = ( Socket * ) vsock;
	net_destroy( &sock->sock );
	sock->in_use = false;
}

static Socket * socket_from_fd( int fd ) {
	for( Socket & sock : sockets ) {
		if( sock.in_use && sock.sock.fd == fd ) {
			return &sock;
		}
	}

	return NULL;
}

struct MudFont {
	int ascent;
	int width, height;
	HFONT font;
};

struct {
	COLORREF bg;
	COLORREF status_bg;
	COLORREF cursor;

	MudFont font;
	MudFont bold_font;

	union {
		struct {
			COLORREF black;
			COLORREF red;
			COLORREF green;
			COLORREF yellow;
			COLORREF blue;
			COLORREF magenta;
			COLORREF cyan;
			COLORREF white;

			COLORREF lblack;
			COLORREF lred;
			COLORREF lgreen;
			COLORREF lyellow;
			COLORREF lblue;
			COLORREF lmagenta;
			COLORREF lcyan;
			COLORREF lwhite;

			COLORREF system;
		} Colours;

		COLORREF colours[ 2 ][ 8 ];
	};
} Style;

static COLORREF get_colour( Colour colour, bool bold ) {
	switch( colour ) {
		case SYSTEM:
			return Style.Colours.system;
		case COLOUR_BG:
			return Style.bg;
		case COLOUR_STATUSBG:
			return Style.status_bg;
		case COLOUR_CURSOR:
			return Style.cursor;
	}

	return Style.colours[ bold ][ colour ];
}

void ui_fill_rect( int left, int top, int width, int height, Colour colour, bool bold ) {
	HBRUSH brush = CreateSolidBrush( get_colour( colour, bold ) );
	RECT r = { left, top, left + width, top + height };
	FillRect( UI.hdc, &r, brush );
	DeleteObject( brush );
}

void ui_draw_char( int left, int top, char c, Colour colour, bool bold, bool force_bold_font ) {
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

	if( uint8_t( c ) == 193 ) { // bottom stopper
		ui_fill_rect( left + left_spacing, top, 1, top_spacing, colour, bold );
		ui_fill_rect( left, top + top_spacing, Style.font.width, 1, colour, bold );
		return;
	}

	if( uint8_t( c ) == 194 ) { // top stopper
		ui_fill_rect( left + left_spacing, top + top_spacing, 1, bot_spacing, colour, bold );
		ui_fill_rect( left, top + top_spacing, Style.font.width, 1, colour, bold );
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

	SelectObject( UI.hdc, ( bold || force_bold_font ? Style.bold_font : Style.font ).font );
	SetTextColor( UI.hdc, get_colour( colour, bold ) );
	TextOutA( UI.hdc, left, top + SPACING, &c, 1 );
}

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
			FATAL( "realloc" );

		statusContents = newcontents;
		statusCapacity = newcapacity;
	}

	statusContents[ statusLen ] = { c, fg, bold };
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
}

void draw_input() {
	InputBuffer input = input_get_buffer();

	int top = UI.height - PADDING - Style.font.height;
	ui_fill_rect( PADDING, top, UI.width - PADDING * 2, Style.font.height, COLOUR_BG, false );

	for( size_t i = 0; i < input.len; i++ )
		ui_draw_char( PADDING + i * Style.font.width, top - SPACING, input.buf[ i ], WHITE, false );

	ui_fill_rect( PADDING + input.cursor_pos * Style.font.width, top, Style.font.width, Style.font.height, COLOUR_CURSOR, false );

	if( input.cursor_pos < input.len ) {
		ui_draw_char( PADDING + input.cursor_pos * Style.font.width, top - SPACING, input.buf[ input.cursor_pos ], COLOUR_BG, false );
	}
}

void ui_draw() {
	ui_fill_rect( 0, 0, UI.width, UI.height, COLOUR_BG, false );

	draw_input();
	ui_draw_status();

	textbox_draw( &UI.chat_text );
	textbox_draw( &UI.main_text );

	int spacerY = ( 2 * PADDING ) + ( Style.font.height + SPACING ) * CHAT_ROWS;
	ui_fill_rect( 0, spacerY, UI.width, 1, COLOUR_STATUSBG, false );
}

void ui_handleXEvents() { }

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

void ui_get_font_size( int * fw, int * fh ) {
	*fw = Style.font.width;
	*fh = Style.font.height;
}

void ui_urgent() {
	FlashWindow( UI.hwnd, FALSE );
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	switch( msg ) {
		case WM_CREATE: {
			UI.hdc = GetDC( hwnd );
			SetBkMode( UI.hdc, TRANSPARENT );

			Style.font.font = CreateFont( 14, 0, 0, 0, FW_REGULAR,
				FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
				"Dina" );
			Style.bold_font.font = CreateFont( 14, 0, 0, 0, FW_BOLD,
				FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
				"Dina" );

			SelectObject( UI.hdc, Style.font.font );

			TEXTMETRIC metrics;
			GetTextMetrics( UI.hdc, &metrics );

			Style.font.height = metrics.tmHeight;
			Style.font.width = metrics.tmMaxCharWidth;
			Style.font.ascent = metrics.tmAscent;

			Style.bg             = RGB( 0x1a, 0x1a, 0x1a );
			Style.status_bg      = RGB( 0x33, 0x33, 0x33 );
			Style.cursor         = RGB( 0x00, 0xff, 0x00 );
			Style.Colours.system = RGB( 0xff, 0xff, 0xff );

			Style.Colours.black   = RGB( 0x1a, 0x1a, 0x1a );
			Style.Colours.red     = RGB( 0xca, 0x44, 0x33 );
			Style.Colours.green   = RGB( 0x17, 0x8a, 0x3a );
			Style.Colours.yellow  = RGB( 0xdc, 0x7c, 0x2a );
			Style.Colours.blue    = RGB( 0x41, 0x5e, 0x87 );
			Style.Colours.magenta = RGB( 0x5e, 0x46, 0x8c );
			Style.Colours.cyan    = RGB( 0x35, 0x78, 0x9b );
			Style.Colours.white   = RGB( 0xb6, 0xc2, 0xc4 );

			Style.Colours.lblack   = RGB( 0x66, 0x66, 0x66 );
			Style.Colours.lred     = RGB( 0xff, 0x29, 0x54 );
			Style.Colours.lgreen   = RGB( 0x5d, 0xd0, 0x30 );
			Style.Colours.lyellow  = RGB( 0xfa, 0xfc, 0x4f );
			Style.Colours.lblue    = RGB( 0x35, 0x81, 0xe1 );
			Style.Colours.lmagenta = RGB( 0x87, 0x5f, 0xff );
			Style.Colours.lcyan    = RGB( 0x29, 0xfb, 0xff );
			Style.Colours.lwhite   = RGB( 0xce, 0xdb, 0xde );
		} break;

		case WM_SIZE: {
			int old_width = UI.width;
			int old_height = UI.height;

			UI.width = LOWORD( lParam );
			UI.height = HIWORD( lParam );

			if( UI.width == old_width && UI.height == old_height )
				break;

			textbox_set_pos( &UI.chat_text, PADDING, PADDING );
			textbox_set_size( &UI.chat_text, UI.width - ( 2 * PADDING ), ( Style.font.height + SPACING ) * CHAT_ROWS );

			textbox_set_pos( &UI.main_text, PADDING, ( PADDING * 2 ) + CHAT_ROWS * ( Style.font.height + SPACING ) + 1 );
			textbox_set_size( &UI.main_text, UI.width - ( 2 * PADDING ), UI.height
				- ( ( ( Style.font.height + SPACING ) * CHAT_ROWS ) + ( PADDING * 2 ) )
				- ( ( Style.font.height * 2 ) + ( PADDING * 5 ) ) - 1
			);

			ui_draw();
		} break;

		case WM_LBUTTONDOWN: {
			// char szFileName[MAX_PATH];
			// HINSTANCE hInstance = GetModuleHandle(NULL);
                        //
			// GetModuleFileName(hInstance, szFileName, MAX_PATH);
			// MessageBox(hwnd, szFileName, "This program is:", MB_OK | MB_ICONINFORMATION);
		} break;

		case WM_MOUSEMOVE: {
			// char buf[ 128 ];
			// int x = GET_X_LPARAM( lParam );
			// int y = GET_Y_LPARAM( lParam );
			// int l = snprintf( buf, sizeof( buf ), "what the fuck son %d %d", x, y );
			// TextOutA( dc, 10, 10, buf, l );
		} break;

		case WM_CLOSE:
			DestroyWindow( hwnd );
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;

		case WM_TIMER: {
			script_fire_intervals();
		} break;

		case WM_CHAR: {
			if( wParam >= ' ' && wParam < 128 ) {
				char c = wParam;
				input_add( &c, 1 );
				ui_draw();
			}
		} break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN: {
			#define ADD_MACRO( key, name ) \
				case key: \
					  script_doMacro( name, sizeof( name ) - 1, shift, ctrl, alt ); \
					break

			bool shift = ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0;
			bool ctrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
			bool alt = ( GetKeyState( VK_MENU ) & 0x8000 ) != 0;

			switch( wParam ) {
				case VK_BACK:
					input_backspace();
					ui_draw();
					break;

				case VK_DELETE:
					input_delete();
					ui_draw();
					break;

				case VK_RETURN:
					input_return();
					ui_draw();
					break;

				case VK_LEFT:
					input_left();
					ui_draw();
					break;

				case VK_RIGHT:
					input_right();
					ui_draw();
					break;

				case VK_UP:
					input_up();
					ui_draw();
					break;

				case VK_DOWN:
					input_down();
					ui_draw();
					break;

				case VK_PRIOR:
					if( shift )
						textbox_scroll( &UI.main_text, 1 );
					else
						textbox_page_up( &UI.main_text );
					textbox_draw( &UI.main_text );
					break;

				case VK_NEXT:
					if( shift )
						textbox_scroll( &UI.main_text, -1 );
					else
						textbox_page_down( &UI.main_text );
					textbox_draw( &UI.main_text );
					break;

				ADD_MACRO( VK_NUMPAD0, "kp0" );
				ADD_MACRO( VK_NUMPAD1, "kp1" );
				ADD_MACRO( VK_NUMPAD2, "kp2" );
				ADD_MACRO( VK_NUMPAD3, "kp3" );
				ADD_MACRO( VK_NUMPAD4, "kp4" );
				ADD_MACRO( VK_NUMPAD5, "kp5" );
				ADD_MACRO( VK_NUMPAD6, "kp6" );
				ADD_MACRO( VK_NUMPAD7, "kp7" );
				ADD_MACRO( VK_NUMPAD8, "kp8" );
				ADD_MACRO( VK_NUMPAD9, "kp9" );

				ADD_MACRO( VK_MULTIPLY, "kp*" );
				ADD_MACRO( VK_DIVIDE, "kp/" );
				ADD_MACRO( VK_SUBTRACT, "kp-" );
				ADD_MACRO( VK_ADD, "kp+" );
				ADD_MACRO( VK_DECIMAL, "kp." );

				ADD_MACRO( VK_F1, "f1" );
				ADD_MACRO( VK_F2, "f2" );
				ADD_MACRO( VK_F3, "f3" );
				ADD_MACRO( VK_F4, "f4" );
				ADD_MACRO( VK_F5, "f5" );
				ADD_MACRO( VK_F6, "f6" );
				ADD_MACRO( VK_F7, "f7" );
				ADD_MACRO( VK_F8, "f8" );
				ADD_MACRO( VK_F9, "f9" );
				ADD_MACRO( VK_F10, "f10" );
				ADD_MACRO( VK_F11, "f11" );
				ADD_MACRO( VK_F12, "f12" );

				default: {
					char c = MapVirtualKeyA( wParam, MAPVK_VK_TO_CHAR );
					if( c == 0 )
						break;
					c = tolower( c );
					if( ctrl || alt ) {
						script_doMacro( &c, 1, shift, ctrl, alt );
					}
				} break;
			}

			#undef ADD_MACRO
		} break;

		case 12345: {
			if( WSAGETSELECTERROR( lParam ) ) {
				printf( "bye\n" );
				break;
			}

			SOCKET fd = ( SOCKET ) wParam;
			Socket * sock = socket_from_fd( fd );
			if( sock == NULL )
				break;

			assert( WSAGETSELECTEVENT( lParam ) == FD_CLOSE || WSAGETSELECTEVENT( lParam ) == FD_READ );

			while( true ) {
				char buf[ 2048 ];
				int n = recv( fd, buf, sizeof( buf ), 0 );
				if( n >= 0 ) {
					script_socketData( sock, n > 0 ? buf : NULL, n );
				}
				else {
					int err = WSAGetLastError();
					if( err != WSAEWOULDBLOCK )
						FATAL( "shit %d", err );
					break;
				}
			}
		} break;

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

static bool is_macro( const MSG * msg ) {
	if( msg->message != WM_KEYDOWN && msg->message != WM_KEYUP
	    && msg->message != WM_SYSKEYDOWN && msg->message != WM_SYSKEYUP )
		return false;

	WPARAM macro_vks[] = {
		VK_NUMPAD0,
		VK_NUMPAD1,
		VK_NUMPAD2,
		VK_NUMPAD3,
		VK_NUMPAD4,
		VK_NUMPAD5,
		VK_NUMPAD6,
		VK_NUMPAD7,
		VK_NUMPAD8,
		VK_NUMPAD9,
		VK_MULTIPLY,
		VK_DIVIDE,
		VK_SUBTRACT,
		VK_ADD,
		VK_DECIMAL,
	};

	for( WPARAM vk : macro_vks ) {
		if( msg->wParam == vk )
			return true;
	}

	bool ctrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
	bool alt = ( GetKeyState( VK_MENU ) & 0x8000 ) != 0;

	return ctrl || alt;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {
	for( Socket & s : sockets ) {
		s.in_use = false;
	}

	UI = { };

	textbox_init( &UI.main_text, SCROLLBACK_SIZE );
	textbox_init( &UI.chat_text, CHAT_ROWS );

	statusContents = ( StatusChar * ) malloc( statusCapacity * sizeof( StatusChar ) );
	if( statusContents == NULL )
		FATAL( "malloc" );

	WNDCLASSEX wc = { };
	wc.cbSize        = sizeof( WNDCLASSEX );
	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = hInstance;
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = ( HBRUSH ) COLOR_WINDOW;
	wc.lpszClassName = WINDOW_CLASSNAME;
	wc.hbrBackground = CreateSolidBrush( RGB( 0x1a, 0x1a, 0x1a ) );

	if( !RegisterClassEx( &wc ) ) {
		MessageBox(NULL, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	UI.hwnd = CreateWindowExA(
		NULL,
		WINDOW_CLASSNAME,
		"Mud Gangster",
		WS_OVERLAPPEDWINDOW | WS_MAXIMIZE | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	if( UI.hwnd == NULL ) {
		MessageBox( NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK );
		return 0;
	}

	ShowWindow( UI.hwnd, SW_MAXIMIZE );
	UpdateWindow( UI.hwnd );
	SetTimer( UI.hwnd, 1, 500, NULL );

	net_init();
	input_init();
	script_init();

	MSG msg;
	while( GetMessage( &msg, NULL, 0, 0 ) > 0 ) {
		if( !is_macro( &msg ) )
			TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	script_term();
	input_term();
	net_term();

	// TODO: clean up UI stuff

	return 0;
}
