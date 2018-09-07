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

	HDC back_buffer;
	HBITMAP back_buffer_bitmap;

	TextBox main_text;
	TextBox chat_text;

	int width, height;
	int max_width, max_height;
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
	bool ok = net_new_tcp( &sock, addr );
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
	HFONT regular;
	HFONT bold;
};

struct {
	COLORREF bg;
	COLORREF status_bg;
	COLORREF cursor;

	MudFont font;
	HFONT dc_font;

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

void platform_fill_rect( int left, int top, int width, int height, Colour colour, bool bold ) {
	// TODO: preallocate these
	HBRUSH brush = CreateSolidBrush( get_colour( colour, bold ) );
	RECT r = { left, top, left + width, top + height };
	FillRect( UI.back_buffer, &r, brush );
	DeleteObject( brush );
}

void platform_draw_char( int left, int top, char c, Colour colour, bool bold, bool force_bold_font ) {
	SelectObject( UI.back_buffer, ( bold || force_bold_font ? Style.font.bold : Style.font.regular ) );
	SetTextColor( UI.back_buffer, get_colour( colour, bold ) );
	TextOutA( UI.back_buffer, left, top + SPACING, &c, 1 );
}

void platform_make_dirty( int left, int top, int width, int height ) {
	RECT r = { left, top, left + width, top + height };
	InvalidateRect( UI.hwnd, &r, FALSE );
}

void ui_get_font_size( int * fw, int * fh ) {
	*fw = Style.font.width;
	*fh = Style.font.height;
}

void ui_urgent() {
	FlashWindow( UI.hwnd, FALSE );
}

bool ui_set_font( const char * name, int size ) {
	HFONT regular = CreateFontA( size, 0, 0, 0, FW_REGULAR,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH,
		name );

	if( regular == NULL )
		return false;

	HFONT bold = CreateFontA( size, 0, 0, 0, FW_BOLD,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH,
		name );

	if( bold == NULL ) {
		DeleteObject( regular );
		return false;
	}

	if( Style.font.regular != NULL ) {
		SelectObject( UI.hdc, Style.dc_font );
		DeleteObject( Style.font.regular );
		DeleteObject( Style.font.bold );
	}

	Style.font.regular = regular;
	Style.font.bold = bold;
	Style.dc_font = ( HFONT ) SelectObject( UI.hdc, Style.font.regular );

	TEXTMETRIC metrics;
	GetTextMetrics( UI.hdc, &metrics );

	Style.font.height = metrics.tmHeight;
	Style.font.width = metrics.tmAveCharWidth;
	Style.font.ascent = metrics.tmAscent;

	ui_draw();
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	switch( msg ) {
		case WM_CREATE: {
			UI.hdc = GetDC( hwnd );
			UI.back_buffer = CreateCompatibleDC( UI.hdc );

			SetBkMode( UI.back_buffer, TRANSPARENT );

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

			ui_set_font( "", 14 );
		} break;

		case WM_SIZE: {
			int width = LOWORD( lParam );
			int height = HIWORD( lParam );

			int old_max_width = UI.max_width;
			int old_max_height = UI.max_height;

			UI.max_width = max( UI.max_width, UI.width );
			UI.max_height = max( UI.max_height, UI.height );

			if( UI.max_width != old_max_width || UI.max_height != old_max_height ) {
				if( old_width != -1 ) {
					DeleteObject( UI.back_buffer_bitmap );
				}
				UI.back_buffer_bitmap = CreateCompatibleBitmap( UI.hdc, UI.max_width, UI.max_height );
				SelectObject( UI.back_buffer, UI.back_buffer_bitmap );
			}

			ui_resized( width, height );
			ui_redraw();
		} break;

		case WM_PAINT: {
			RECT r;
			GetUpdateRect( UI.hwnd, &r, FALSE );
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( UI.hwnd, &ps );
			BitBlt( UI.hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, UI.back_buffer, r.left, r.top, SRCCOPY );
			EndPaint( UI.hwnd, &ps );
		} break;

		case WM_ERASEBKGND:
			return TRUE;

		case WM_LBUTTONDOWN: {
			ui_mouse_down( GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) );
		} break;

		case WM_MOUSEMOVE: {
			ui_mouse_move( GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) );
			break;

		case WM_LBUTTONUP: {
			ui_mouse_up( GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) );
		} break;

		case WM_CLOSE: {
			DestroyWindow( hwnd );
		} break;

		case WM_DESTROY: {
			PostQuitMessage( 0 );
		} break;

		case WM_TIMER: {
			script_fire_intervals();
		} break;

		case WM_CHAR: {
			if( wParam >= ' ' && wParam < 127 ) {
				char c = char( wParam );
				input_add( &c, 1 );
				draw_input();
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
					draw_input();
					break;

				case VK_DELETE:
					input_delete();
					draw_input();
					break;

				case VK_RETURN:
					input_return();
					draw_input();
					break;

				case VK_LEFT:
					input_left();
					draw_input();
					break;

				case VK_RIGHT:
					input_right();
					draw_input();
					break;

				case VK_UP:
					input_up();
					draw_input();
					break;

				case VK_DOWN:
					input_down();
					draw_input();
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
				if( n > 0 ) {
					script_socketData( sock, buf, n );
				}
				else if( n == 0 ) {
					script_socketData( sock, NULL, n );
					break;
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
			return DefWindowProc( hwnd, msg, wParam, lParam );
	}

	if( UI.main_text.dirty )
		textbox_draw( &UI.main_text );
	if( UI.chat_text.dirty )
		textbox_draw( &UI.chat_text );

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
	Style = { };

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
