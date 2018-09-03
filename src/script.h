#ifndef _SCRIPT_H_
#define _SCRIPT_H_

void script_handleInput( const char * buffer, int len );
void script_doMacro( const char * key, int len, bool shift, bool ctrl, bool alt );
void script_handleClose();

void script_init();
void script_end();

#endif // _SCRIPT_H_
