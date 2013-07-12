#ifndef _COMMON_H_
#define _COMMON_H_

typedef enum { false, true } bool;

#include <X11/Xlib.h>

#include "config.h"
#include "textbox.h"

struct
{
	Display* display;
	int screen;

	GC gc;
	Colormap colorMap;

	Window window;

	TextBox* textMain;
	TextBox* textChat;

	int width;
	int height;

	int hasFocus;
} UI;

typedef struct
{
	int ascent;
	int descent;

	short lbearing;
	short rbearing;

	int height;
	int width;

	XFontStruct* font;
} MudFont;

struct
{
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

	union
	{
		struct
		{
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

#endif // _COMMON_H_

#define PRETEND_TO_USE( x ) ( void ) ( x )
#define STRL( x ) ( x ), sizeof( x ) - 1

#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
