#ifndef _SCRIPT_H_
#define _SCRIPT_H_

void script_handleInput( char* buffer, int len );
void script_doMacro( char* key, int len, bool shift, bool ctrl, bool alt );
void script_handleClose();

void script_init();
void script_end();

#endif // _SCRIPT_H_
