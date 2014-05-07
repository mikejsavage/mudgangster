#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <X11/Xlib.h>

#include "common.h"
#include <stdio.h>

TextBox* textbox_new( unsigned int maxLines )
{
	TextBox* textbox = calloc( sizeof( TextBox ), 1 );

	assert( textbox != NULL );

	textbox->lines = malloc( maxLines * sizeof( Line ) );
	textbox->numLines = -1;
	textbox->maxLines = maxLines;
	textbox->scrollDelta = 0;

	Line* line = &textbox->lines[ 0 ];

	line->head = line->tail = calloc( sizeof( Text ), 1 );

	return textbox;
}

void textbox_freeline( TextBox* self, int idx )
{
	Text* node = self->lines[ idx ].head;

	while( node != NULL )
	{
		Text* next = node->next;

		free( node->buffer );
		free( node );

		node = next;
	}
}

void textbox_free( TextBox* self )
{
	for( int i = 0; i < self->numLines; i++ )
	{
		textbox_freeline( self, i );
	}

	free( self );
}

void textbox_setpos( TextBox* self, int x, int y )
{
	self->x = x;
	self->y = y;
}

void textbox_setsize( TextBox* self, int width, int height )
{
	self->width = width;
	self->height = height;

	self->rows = height / ( Style.font.height + SPACING );
	self->cols = width / Style.font.width;
}

void textbox_add( TextBox* self, const char* str, unsigned int len, Colour fg, Colour bg, bool bold )
{
	if( self->numLines == -1 )
	{
		self->numLines = 0;
	}

	Line* line = &self->lines[ ( self->head + self->numLines ) % self->maxLines ];

	Text* node = malloc( sizeof( Text ) );
	node->buffer = malloc( len );

	memcpy( node->buffer, str, len );
	node->len = len;

	node->fg = fg;
	node->bg = bg;
	node->bold = bold;

	node->next = NULL;

	line->tail->next = node;
	line->tail = node;

	line->len += len;
}

void textbox_newline( TextBox* self )
{
	if( self->numLines < self->maxLines )
	{
		self->numLines++;

		Line* line = &self->lines[ ( self->head + self->numLines ) % self->maxLines ];

		line->head = line->tail = calloc( sizeof( Text ), 1 );

		if( self->scrollDelta != 0 )
		{
			self->scrollDelta++;
		}
	}
	else
	{
		textbox_freeline( self, self->head );

		Line* line = &self->lines[ self->head ];

		line->head = line->tail = calloc( sizeof( Text ), 1 );

		self->head = ( self->head + 1 ) % self->maxLines;
	}
}

void textbox_draw( TextBox* self )
{
	if( self->width == 0 || self->height == 0 ) {
		return;
	}

	Pixmap doublebuf = XCreatePixmap( UI.display, UI.window, self->width, self->height, UI.depth );

	XSetForeground( UI.display, UI.gc, Style.bg );
	XFillRectangle( UI.display, doublebuf, UI.gc, 0, 0, self->width, self->height );

	int rowsRemaining = self->rows;
	int linesRemaining = self->numLines - self->scrollDelta + 1;

	int lineTop = self->height;
	int linesPrinted = 0;

	int firstLine = self->head + self->numLines - self->scrollDelta;

	while( rowsRemaining > 0 && linesRemaining > 0 )
	{
		Line* line = &self->lines[ ( firstLine - linesPrinted ) % self->maxLines ];

		int lineHeight = ( line->len / self->cols );

		if( line->len % self->cols != 0 || line->len == 0 )
		{
			lineHeight++;
		}

		lineTop -= lineHeight * ( Style.font.height + SPACING );

		Text* node = line->head->next;
		int row = 0;
		int col = 0;

		while( node != NULL )
		{
			if( node->len != 0 )
			{
				XSetFont( UI.display, UI.gc, ( node->bold ? Style.fontBold : Style.font ).font->fid );
				XSetForeground( UI.display, UI.gc,
					node->fg == SYSTEM ? Style.Colours.system : Style.colours[ node->bold ][ node->fg ] );

				// TODO: draw bg

				int remainingChars = node->len;
				int pos = 0;

				while( remainingChars > 0 )
				{
					int charsToPrint = MIN( remainingChars, self->cols - col );

					int x = ( Style.font.width * col );
					int y = lineTop + ( ( Style.font.height + SPACING ) * row );

					if( y >= 0 )
					{
						XDrawString( UI.display, doublebuf, UI.gc, x, y + Style.font.ascent + SPACING, node->buffer + pos, charsToPrint );
					}

					pos += charsToPrint;
					remainingChars -= charsToPrint;

					if( remainingChars != 0 )
					{
						col = 0;
						row++;
					}
					else
					{
						col += charsToPrint;
					}
				}
			}

			node = node->next;
		}

		rowsRemaining -= lineHeight;
		linesRemaining--;

		linesPrinted++;
	}

	XCopyArea( UI.display, doublebuf, UI.window, UI.gc, 0, 0, self->width, self->height, self->x, self->y );
	XFreePixmap( UI.display, doublebuf );
}
