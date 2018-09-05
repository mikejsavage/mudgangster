#include "script.h"
#include "input.h"
#include "ui.h"

int main() {
	ui_init();
	input_init();
	script_init();

	event_loop();

	script_term();
	input_term();
	ui_term();

	return 0;
}
