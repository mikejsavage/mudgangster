#pragma once

#include <X11/Xlib.h>

#include "config.h"
#include "textbox.h"

struct UIDefs {
	Display * display;
	int screen;

	GC gc;
	Colormap colorMap;

	Window window;

	TextBox textMain;
	TextBox textChat;

	int width;
	int height;
	int depth;

	int hasFocus;
};

extern UIDefs UI;

struct MudFont {
	int ascent;
	int descent;

	short lbearing;
	short rbearing;

	int height;
	int width;

	XFontStruct * font;
};

struct StyleDefs {
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
};

extern StyleDefs Style;

#define STRL( x ) ( x ), sizeof( x ) - 1

template< typename T >
constexpr T min( T a, T b ) {
	return a < b ? a : b;
}

template< typename T >
constexpr T max( T a, T b ) {
	return a > b ? a : b;
}
