#include "gui.h"
#define UI_WINDOWS
#include <luigi.h>
#include <string.h>
#include <stdio.h>
#include "macros/macros.h"

enum
{	BUTTON_ENABLE,
	BUTTON_DISABLE,
	BUTTON_TOGGLE,
	BUTTON_MAX
};

static bool state = false;
static gui_state_changed_callback state_changed_callback;
static UIWindow * window;
static UIPanel * main_panel;
static UILabel * label;
static UIButton * state_buttons[BUTTON_MAX];
static UICheckbox * topmost_checkbox;
static WNDPROC luigi_window_procedure;

static void create_window(void);
static void create_label(void);
static void create_state_buttons(void);
static void create_topmost_checkbox(void);
static void refresh_label(void);
static void refresh_state_buttons(void);
static void refresh_topmost_state(void);
static void change_state(bool new_state);
static int state_button_message_callback(UIElement *element, UIMessage message, int di, void *dp);
static int topmost_checkbox_message_callback(UIElement *element, UIMessage message, int di, void *dp);
static void hook_luigi_window_procedure(void);
static void register_hotkeys(void);
static LRESULT CALLBACK hooked_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

int gui_run(gui_state_changed_callback state_changed_callback_argument)
{	state_changed_callback = state_changed_callback_argument;
	UIInitialise();
	create_window();
	create_label();
	create_state_buttons();
	create_topmost_checkbox();
	hook_luigi_window_procedure();
	register_hotkeys();
	return UIMessageLoop();

}

static void create_window(void)
{	window = UIWindowCreate(NULL, 0, "Replay Glitch Helper 2", 400, 200);
	main_panel = UIPanelCreate(window, UI_PANEL_GRAY | UI_PANEL_MEDIUM_SPACING | UI_PANEL_SCROLL);
}

static void create_label(void)
{	UIPanel * label_panel = UIPanelCreate(main_panel, UI_PANEL_GRAY | UI_PANEL_HORIZONTAL | UI_ELEMENT_H_FILL);
	UISpacerCreate(label_panel, 0, 10, 0);
	label = UILabelCreate(label_panel, UI_ELEMENT_H_FILL, "", -1);
	refresh_label();
}

static void create_state_buttons(void)
{	char * texts[BUTTON_MAX] = { [BUTTON_ENABLE] = "Enable (F5)", [BUTTON_DISABLE] = "Disable (F6)", [BUTTON_TOGGLE] = "Toggle (F7)" };
	UIPanel * button_panel = UIPanelCreate(main_panel, UI_PANEL_GRAY | UI_PANEL_HORIZONTAL | UI_ELEMENT_H_FILL);
	for(int i = 0; i < BUTTON_MAX; ++i)
	{	UIButton * button = UIButtonCreate(button_panel, 0, texts[i], -1);
		button->e.messageUser = state_button_message_callback;
		state_buttons[i] = button;
	}
	UISpacerCreate(button_panel, UI_ELEMENT_H_FILL, 0, 0);
	refresh_state_buttons();
}

static void create_topmost_checkbox(void)
{	topmost_checkbox = UICheckboxCreate(main_panel, UI_ELEMENT_H_FILL, "Always On Top", -1);
	topmost_checkbox->e.messageUser = topmost_checkbox_message_callback;
	refresh_topmost_state();
}

static void refresh_label(void)
{	enum { text_size = 20 };
	static char text[text_size];
	int text_length = snprintf(NULL, 0, "Currently %s", state ? "ON" : "OFF");
	assert(text_length > 0 && text_length < text_size);
	assert(sprintf(text, "Currently %s", state ? "ON" : "OFF") == text_length);
	label->label = text;
	label->labelBytes = strlen(text);
	UIElementRefresh(label);
}

static void refresh_state_buttons(void)
{	bool states[] = { [BUTTON_ENABLE] = true, [BUTTON_DISABLE] = false };
	for(int i = 0; i < sizeof(states) / sizeof(states[0]); ++i)
	{	uint32_t * flags = &state_buttons[i]->e.flags;
		*flags = (*flags & ~UI_ELEMENT_DISABLED) | (state ^ states[i] ? 0 : UI_ELEMENT_DISABLED);
		UIElementRefresh(state_buttons[i]);
	}
}

static void refresh_topmost_state(void)
{	assert(SetWindowPos(window->hwnd, topmost_checkbox->check ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE));
}

static void change_state(bool new_state)
{	state = new_state;
	refresh_label();
	refresh_state_buttons();
	state_changed_callback(state);
}

static int state_button_message_callback(UIElement *element, UIMessage message, int di, void *dp)
{	if(message == UI_MSG_CLICKED)
	{	int button_id = -1;
		for(int i = 0; i < BUTTON_MAX; ++i)
			if(state_buttons[i] == (void*)element)
			{	button_id = i;
				break;
			}
		assert(button_id != -1);
		switch(button_id)
		{	case BUTTON_ENABLE: change_state(true); break;
			case BUTTON_DISABLE: change_state(false); break;
			case BUTTON_TOGGLE: change_state(!state); break;
			default: verbose_abort("unknown button id");
		}
	}
	return 0;
}

static int topmost_checkbox_message_callback(UIElement * element, UIMessage message, int di, void * dp)
{	if(message == UI_MSG_LEFT_UP)
		refresh_topmost_state();
	return 0;
}

static void hook_luigi_window_procedure(void)
{	luigi_window_procedure = (WNDPROC)GetWindowLongPtrA(window->hwnd, GWLP_WNDPROC);
	assert(luigi_window_procedure);
	assert(SetWindowLongPtrA(window->hwnd, GWLP_WNDPROC, (LONG_PTR)hooked_window_procedure));
}

static void register_hotkeys(void)
{	assert(RegisterHotKey(window->hwnd, BUTTON_ENABLE, MOD_NOREPEAT, VK_F5));
	assert(RegisterHotKey(window->hwnd, BUTTON_DISABLE, MOD_NOREPEAT, VK_F6));
	assert(RegisterHotKey(window->hwnd, BUTTON_TOGGLE, MOD_NOREPEAT, VK_F7));
}

static LRESULT CALLBACK hooked_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{	if(message == WM_HOTKEY)
	{	UIButton * button = state_buttons[wparam];
		UIElementMessage(button, UI_MSG_CLICKED, 0, NULL);
		/**
		 * note :
		 * since luigi only redraws GUIs on received inputs, forcing this behaviour is required here.
		 * this is the only way to get luigi to redraw all GUIs that doesn't require additional and pointless (under this context) luigi code to be executed.
		 * other ways mostly imply using SendMessage to send bogus inputs (e.g. mouse move by 0 pixels) to luigi's win32 window which luigi will react to by
		 * calling _UIUpdate itself.
		 * I guess nakst doesn't know how static works.
		 */
		void _UIUpdate(void);
		_UIUpdate();
	}
	return luigi_window_procedure(window, message, wparam, lparam);
}
