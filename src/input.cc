#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "input.h"
#include "script.h"

typedef struct {
	char * text;
	int len;
} InputHistory;

static InputHistory inputHistory[ MAX_INPUT_HISTORY ];
static int inputHistoryHead = 0;
static int inputHistoryCount = 0;
static int inputHistoryDelta = 0;

static char * inputBuffer = NULL;
static char * starsBuffer = NULL;

static int inputBufferSize = 256;

static int inputLen = 0;
static int inputPos = 0;

void input_init() {
	inputBuffer = ( char * ) malloc( inputBufferSize );
	starsBuffer = ( char * ) malloc( inputBufferSize );

	memset( starsBuffer, '*', inputBufferSize );
}

void input_term() {
	free( inputBuffer );
	free( starsBuffer );
}

InputBuffer input_get_buffer() {
	InputBuffer buf;
	buf.buf = inputBuffer;
	buf.len = inputLen;
	buf.cursor_pos = inputPos;
	return buf;
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
	inputPos = 0;
}

void input_backspace() {
	if( inputPos > 0 ) {
		memmove( inputBuffer + inputPos - 1, inputBuffer + inputPos, inputLen - inputPos );

		inputLen--;
		inputPos--;
	}
}

void input_delete() {
	if( inputPos < inputLen ) {
		memmove( inputBuffer + inputPos, inputBuffer + inputPos + 1, inputLen - inputPos );

		inputLen--;
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
	inputPos = cmd.len;
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
		inputPos = cmd.len;
	}
	else {
		inputLen = 0;
		inputPos = 0;
	}
}

void input_left() {
	inputPos = max( inputPos - 1, 0 );
}

void input_right() {
	inputPos = min( inputPos + 1, inputLen );
}

void input_add( const char * buffer, int len ) {
	if( inputLen + len >= inputBufferSize ) {
		inputBufferSize *= 2;

		inputBuffer = ( char * ) realloc( inputBuffer, inputBufferSize );
		starsBuffer = ( char * ) realloc( starsBuffer, inputBufferSize );

		memset( starsBuffer + inputBufferSize / 2, '*', inputBufferSize / 2 );
	}

	if( inputPos < inputLen ) {
		memmove( inputBuffer + inputPos + len, inputBuffer + inputPos, inputLen - inputPos );
	}

	memcpy( inputBuffer + inputPos, buffer, len );

	inputLen += len;
	inputPos += len;
}
