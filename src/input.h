#ifndef _INPUT_H_
#define _INPUT_H_

void input_send();

void input_backspace();
void input_delete();

void input_up();
void input_down();
void input_left();
void input_right();

void input_add( const char * buffer, int len );

void input_draw();

void input_init();
void input_end();

#endif // _INPUT_H_
