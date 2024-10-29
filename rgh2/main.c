#include <stdbool.h>
#include "gui/gui.h"
#include "macros/macros.h"
#include "utils/utils.h"
#include "packet_dropper/packet_dropper.h"

void state_changed_callback(bool new_state);

int main(void)
{	if(!is_elevated())
		restart_elevated();
	single_instance_check();
	packet_dropper_setup();
	int result = gui_run(state_changed_callback);
	packet_dropper_cleanup();
	return result;
}

void state_changed_callback(bool new_state)
{	new_state ? packet_dropper_start() : packet_dropper_stop();
}
