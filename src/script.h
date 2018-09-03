#pragma once

// TODO: should be size_t here?
void script_handleInput( const char * buffer, int len );
void script_doMacro( const char * key, int len, bool shift, bool ctrl, bool alt );
void script_handleClose();

void script_init();
void script_term();
