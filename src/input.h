#pragma once

#include <stddef.h>

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

void input_set_pos( int x, int y );
void input_set_size( int w, int h );
bool input_is_dirty();
void input_draw();
