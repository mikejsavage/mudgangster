#include "script.h"
#include "input.h"
#include "ui.h"

int main() {
	ui_init();
	input_init();
	script_init();

	// main loop is done in lua

	script_end();
	input_end();
	ui_end();

	return 0;
}
