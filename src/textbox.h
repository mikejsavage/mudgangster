#ifndef _TEXTBOX_H_
#define _TEXTBOX_H_

typedef enum
{
	BLACK = 0,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	SYSTEM,
	NONE,
} Colour;

typedef struct Text Text;
struct Text
{
	char* buffer;
	unsigned int len;

	Colour fg;
	Colour bg;
	bool bold;

	Text* next;
};

typedef struct
{
	Text* head;
	Text* tail;

	unsigned int len;
} Line;

typedef struct
{
	Line* lines;

	int maxLines;
	int numLines;
	int head;

	int x;
	int y;
	int width;
	int height;

	int rows;
	int cols;

	int scrollDelta;
} TextBox;

TextBox* textbox_new( unsigned int maxLines );
void textbox_free( TextBox* self );
void textbox_setpos( TextBox* self, int x, int y );
void textbox_setsize( TextBox* self, int width, int height );
void textbox_add( TextBox* self, const char* str, unsigned int len, Colour fg, Colour bg, bool bold );
void textbox_newline( TextBox* self );
void textbox_draw( TextBox* self );

#endif // _TEXTBOX_H_
