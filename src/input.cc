#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "input.h"
#include "script.h"
#include "ui.h"

typedef struct {
	char * text;
	size_t len;
} InputHistory;

static InputHistory inputHistory[ MAX_INPUT_HISTORY ];
static int inputHistoryHead = 0;
static int inputHistoryCount = 0;
static int inputHistoryDelta = 0;

static char * inputBuffer = NULL;
static char * starsBuffer = NULL;

static size_t inputBufferSize = 256;

static size_t inputLen = 0;
static size_t cursor_pos = 0;

static int left, top;
static int width, height;

static bool dirty = false;

void input_init() {
	inputBuffer = ( char * ) malloc( inputBufferSize );
	starsBuffer = ( char * ) malloc( inputBufferSize );

	memset( starsBuffer, '*', inputBufferSize );
}

void input_term() {
	free( inputBuffer );
	free( starsBuffer );
}

void input_return() {
	if( inputLen > 0 ) {
		InputHistory * lastCmd = &inputHistory[ ( inputHistoryHead + inputHistoryCount - 1 ) % MAX_INPUT_HISTORY ];

		if( inputLen != lastCmd->len || strncmp( inputBuffer, lastCmd->text, inputLen ) != 0 ) {
			int pos = ( inputHistoryHead + inputHistoryCount ) % MAX_INPUT_HISTORY;

			if( inputHistoryCount == MAX_INPUT_HISTORY ) {
				free( inputHistory[ pos ].text );

				inputHistoryHead = ( inputHistoryHead + 1 ) % MAX_INPUT_HISTORY;
			}
			else {
				inputHistoryCount++;
			}

			inputHistory[ pos ].text = ( char * ) malloc( inputLen );

			memcpy( inputHistory[ pos ].text, inputBuffer, inputLen );
			inputHistory[ pos ].len = inputLen;
		}
	}

	script_handleInput( inputBuffer, inputLen );

	inputHistoryDelta = 0;

	inputLen = 0;
	cursor_pos = 0;

	dirty = true;
}

void input_backspace() {
	if( cursor_pos > 0 ) {
		memmove( inputBuffer + cursor_pos - 1, inputBuffer + cursor_pos, inputLen - cursor_pos );

		inputLen--;
		cursor_pos--;
		dirty = true;
	}
}

void input_delete() {
	if( cursor_pos < inputLen ) {
		memmove( inputBuffer + cursor_pos, inputBuffer + cursor_pos + 1, inputLen - cursor_pos );

		inputLen--;
		dirty = true;
	}
}

void input_up() {
	if( inputHistoryDelta >= inputHistoryCount )
		return;

	inputHistoryDelta++;
	int pos = ( inputHistoryHead + inputHistoryCount - inputHistoryDelta ) % MAX_INPUT_HISTORY;

	InputHistory cmd = inputHistory[ pos ];

	memcpy( inputBuffer, cmd.text, cmd.len );

	inputLen = cmd.len;
	cursor_pos = cmd.len;
	dirty = true;
}

void input_down() {
	if( inputHistoryDelta == 0 )
		return;

	inputHistoryDelta--;

	if( inputHistoryDelta != 0 ) {
		int pos = ( inputHistoryHead + inputHistoryCount - inputHistoryDelta ) % MAX_INPUT_HISTORY;

		InputHistory cmd = inputHistory[ pos ];

		memcpy( inputBuffer, cmd.text, cmd.len );

		inputLen = cmd.len;
		cursor_pos = cmd.len;
	}
	else {
		inputLen = 0;
		cursor_pos = 0;
	}

	dirty = true;
}

void input_left() {
	if( cursor_pos > 0 ) {
		cursor_pos--;
		dirty = true;
	}
}

void input_right() {
	cursor_pos = min( cursor_pos + 1, inputLen );
	dirty = true;
}

void input_add( const char * buffer, int len ) {
	if( inputLen + len >= inputBufferSize ) {
		inputBufferSize *= 2;

		inputBuffer = ( char * ) realloc( inputBuffer, inputBufferSize );
		starsBuffer = ( char * ) realloc( starsBuffer, inputBufferSize );

		memset( starsBuffer + inputBufferSize / 2, '*', inputBufferSize / 2 );
	}

	if( cursor_pos < inputLen ) {
		memmove( inputBuffer + cursor_pos + len, inputBuffer + cursor_pos, inputLen - cursor_pos );
	}

	memcpy( inputBuffer + cursor_pos, buffer, len );

	inputLen += len;
	cursor_pos += len;

	dirty = true;
}

void input_set_pos( int x, int y ) {
	left = x;
	top = y;
}

void input_set_size( int w, int h ) {
	width = w;
	height = h;
}

bool input_is_dirty() {
	return dirty;
}

void input_draw() {
	int fw, fh;
	ui_get_font_size( &fw, &fh );

	ui_fill_rect( left, top, width, height, COLOUR_BG, false );

	for( size_t i = 0; i < inputLen; i++ ) {
		ui_draw_char( PADDING + i * fw, top - SPACING, inputBuffer[ i ], WHITE, false );
	}

	ui_fill_rect( PADDING + cursor_pos * fw, top, fw, fh, COLOUR_CURSOR, false );

	if( cursor_pos < inputLen ) {
		ui_draw_char( PADDING + cursor_pos * fw, top - SPACING, inputBuffer[ cursor_pos ], COLOUR_BG, false );
	}

	dirty = false;
}
