#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include "common.h"
#include "script.h"
#include "input.h"
#include "ui.h"

int main()
{
	ui_init();
	input_init();
	script_init();

	// main loop is done in lua

	script_end();
	input_end();
	ui_end();

	return EXIT_SUCCESS;
}
