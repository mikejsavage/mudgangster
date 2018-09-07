#include "script.h"
#include "input.h"
#include "ui.h"
#include "platform_ui.h"
#include "platform_network.h"

int main() {
	net_init();
	ui_init();
	input_init();
	platform_ui_init();
	script_init();

	event_loop();

	script_term();
	platform_ui_term();
	input_term();
	ui_term();
	net_term();

	return 0;
}
