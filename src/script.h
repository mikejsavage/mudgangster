#pragma once

#include <stddef.h>

// TODO: should be size_t here?
void script_handleInput( const char * buffer, int len );
void script_doMacro( const char * key, int len, bool shift, bool ctrl, bool alt );
void script_handleClose();
void script_socketData( void * sock, const char * data, size_t len );
void script_fire_intervals();

void script_init();
void script_term();
