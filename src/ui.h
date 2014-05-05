#ifndef _UI_H_
#define _UI_H_

#include "common.h"

void ui_handleXEvents();

void ui_statusDraw();
void ui_statusClear();
void ui_statusAdd( const char c, const Colour fg, const bool bold );

void ui_draw();

void ui_init();
void ui_end();

#endif // _UI_H_
