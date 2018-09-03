#pragma once

#include <stddef.h>

struct InputBuffer {
	char * buf;

	size_t len;
	size_t cursor_pos;
};

void input_init();
void input_term();

void input_return();
void input_backspace();
void input_delete();

void input_up();
void input_down();
void input_left();
void input_right();

void input_add( const char * buffer, int len );

InputBuffer input_get_buffer();
