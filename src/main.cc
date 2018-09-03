#include "script.h"
#include "input.h"
#include "ui.h"

int main() {
	ui_init();
	input_init();
	script_init();

	// main loop is done in lua

	script_term();
	input_term();
	ui_term();

	return 0;
}
