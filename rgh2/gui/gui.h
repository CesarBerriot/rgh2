#include <stdbool.h>

typedef void(*gui_state_changed_callback)(bool new_state);

int gui_run(gui_state_changed_callback);
