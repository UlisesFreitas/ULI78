#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "uli.h"
#include "libretro-common/include/libretro.h"
#include "retro_inline.h"
#include "retro_endianness.h"
#include "libretro_core_options.h"
#include "api.h"

/**
 * system.h is used for:
 * - ULI78_OFFSET_LEFT
 * - ULI78_OFFSET_TOP,
 * - ULI_NAME
 * - ULI_VERSION
 */
#include "studio/system.h"

static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

#define RETRO_ANALOG_RANGE 0x8000
#define RETRO_BASE_POINTER_SPEED_PHYSICAL 0.4f
#define RETRO_BASE_POINTER_SPEED_ANALOG 4.3f
#define RETRO_BASE_POINTER_SPEED_DPAD 1.6f
#define RETRO_SLOW_MOUSE_FACTOR_ANALOG 0.3f
#define RETRO_SLOW_MOUSE_FACTOR_DPAD 0.4f
#ifndef ULI78_FREQUENCY
#define ULI78_FREQUENCY 1000000
#endif

enum pointer_device_type
{
	POINTER_DEVICE_MOUSE = 0,
	POINTER_DEVICE_TOUCHSCREEN,
	POINTER_DEVICE_LEFT_ANALOG,
	POINTER_DEVICE_RIGHT_ANALOG,
	POINTER_DEVICE_DPAD
};

enum mouse_cursor_type
{
	MOUSE_CURSOR_NONE = 0,
	MOUSE_CURSOR_DOT,
	MOUSE_CURSOR_CROSS,
	MOUSE_CURSOR_ARROW
};

struct uli78_state
{
	bool quit;
	uli78_input input;
	int keymap[RETROK_LAST];
	bool cropBorder;
	enum pointer_device_type pointerDevice;
	float pointerSpeed;
	bool slowGamepadMouse;
	enum mouse_cursor_type mouseCursor;
	u8 mouseCursorColor;
	int analogDeadzone;
	u16 mouseX;
	u16 mouseY;
	u16 mousePreviousX;
	u16 mousePreviousY;
	float mouseXAccumulator;
	float mouseYAccumulator;
	int mouseHideTimer;
	int mouseHideTimerStart;
	uli78* uli;
	retro_usec_t frameTime;
};
static struct uli78_state* state = NULL;

/**
 * ULI-78 callback; Request counter.
 */
static u64 uli78_libretro_counter()
{
	if (state == NULL) {
		return 0;
	}

	return (u64)state->frameTime;
}

/**
 * ULI-78 callback; Request frequency.
 */
static u64 uli78_libretro_frequency()
{
	return ULI78_FREQUENCY;
}

/**
 * ULI-78 callback; Requests the content to exit.
 */
void uli78_libretro_exit()
{
	if (state == NULL) {
		return;
	}

	state->quit = true;
}

/**
 * ULI-78 callback; Report an error from the ULI-78 cart.
 */
void uli78_libretro_error(const char* info)
{
	// Report the error to the log.
	log_cb(RETRO_LOG_ERROR, "[ULI-78]: %s\n", info);

	// Display the error on the screen, if possible.
	if (environ_cb) {
		struct retro_message msg = {
			info,
			6 * ULI78_FRAMERATE
		};
		environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
	}

	// Finally, on an error, close the core.
	uli78_libretro_exit();
}

/**
 * ULI-78 callback; Report a trace log from the ULI-78 cart.
 */
void uli78_libretro_trace(const char* text, u8 color)
{
	ULI_UNUSED(color);
	log_cb(RETRO_LOG_DEBUG, "[ULI-78] %s\n", text);
}

/**
 * libretro callback; Handles the logging internally when the logging isn't set.
 */
void uli78_libretro_fallback_log(enum retro_log_level level, const char *fmt, ...)
{
	ULI_UNUSED(level);
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

/**
 * libretro callback; Called to indicate how much time has passed since last retro_run().
 */
void uli78_libretro_frame_time(retro_usec_t usec) {
	if (state == NULL) {
		return;
	}

	state->frameTime += usec;
}

/**
 * libretro callback; Global initialization.
 */
RETRO_API void retro_init(void)
{
	// Do not re-initialize.
	if (state != NULL) {
		return;
	}

	// Initialize the base state with some default values.
	state = (struct uli78_state*) malloc(sizeof(struct uli78_state));
	memset(state, 0, sizeof(struct uli78_state));
	state->pointerSpeed = 1.0f;
	state->mouseCursorColor = 15;
	state->analogDeadzone = (int)(0.15f * (float)RETRO_ANALOG_RANGE);

	// Initialize the keyboard mappings.
	state->keymap[RETROK_UNKNOWN] = uli_key_unknown;
	state->keymap[RETROK_a] = uli_key_a;
	state->keymap[RETROK_b] = uli_key_b;
	state->keymap[RETROK_c] = uli_key_c;
	state->keymap[RETROK_d] = uli_key_d;
	state->keymap[RETROK_e] = uli_key_e;
	state->keymap[RETROK_f] = uli_key_f;
	state->keymap[RETROK_g] = uli_key_g;
	state->keymap[RETROK_h] = uli_key_h;
	state->keymap[RETROK_i] = uli_key_i;
	state->keymap[RETROK_j] = uli_key_j;
	state->keymap[RETROK_k] = uli_key_k;
	state->keymap[RETROK_l] = uli_key_l;
	state->keymap[RETROK_m] = uli_key_m;
	state->keymap[RETROK_n] = uli_key_n;
	state->keymap[RETROK_o] = uli_key_o;
	state->keymap[RETROK_p] = uli_key_p;
	state->keymap[RETROK_q] = uli_key_q;
	state->keymap[RETROK_r] = uli_key_r;
	state->keymap[RETROK_s] = uli_key_s;
	state->keymap[RETROK_t] = uli_key_t;
	state->keymap[RETROK_u] = uli_key_u;
	state->keymap[RETROK_v] = uli_key_v;
	state->keymap[RETROK_w] = uli_key_w;
	state->keymap[RETROK_x] = uli_key_x;
	state->keymap[RETROK_y] = uli_key_y;
	state->keymap[RETROK_z] = uli_key_z;
	state->keymap[RETROK_0] = uli_key_0;
	state->keymap[RETROK_1] = uli_key_1;
	state->keymap[RETROK_2] = uli_key_2;
	state->keymap[RETROK_3] = uli_key_3;
	state->keymap[RETROK_4] = uli_key_4;
	state->keymap[RETROK_5] = uli_key_5;
	state->keymap[RETROK_6] = uli_key_6;
	state->keymap[RETROK_7] = uli_key_7;
	state->keymap[RETROK_8] = uli_key_8;
	state->keymap[RETROK_9] = uli_key_9;
	state->keymap[RETROK_KP0] = uli_key_0;
	state->keymap[RETROK_KP1] = uli_key_1;
	state->keymap[RETROK_KP2] = uli_key_2;
	state->keymap[RETROK_KP3] = uli_key_3;
	state->keymap[RETROK_KP4] = uli_key_4;
	state->keymap[RETROK_KP5] = uli_key_5;
	state->keymap[RETROK_KP6] = uli_key_6;
	state->keymap[RETROK_KP7] = uli_key_7;
	state->keymap[RETROK_KP8] = uli_key_8;
	state->keymap[RETROK_KP9] = uli_key_9;
	state->keymap[RETROK_MINUS] = uli_key_minus;
	state->keymap[RETROK_EQUALS] = uli_key_equals;
	state->keymap[RETROK_LEFTBRACKET] = uli_key_leftbracket;
	state->keymap[RETROK_RIGHTBRACKET] = uli_key_rightbracket;
	state->keymap[RETROK_BACKSLASH] = uli_key_backslash;
	state->keymap[RETROK_SEMICOLON] = uli_key_semicolon;
	state->keymap[RETROK_QUOTE] = uli_key_apostrophe;
	state->keymap[RETROK_TILDE] = uli_key_grave;
	state->keymap[RETROK_COMMA] = uli_key_comma;
	state->keymap[RETROK_PERIOD] = uli_key_period;
	state->keymap[RETROK_SLASH] = uli_key_slash;
	state->keymap[RETROK_SPACE] = uli_key_space;
	state->keymap[RETROK_TAB] = uli_key_tab;
	state->keymap[RETROK_RETURN] = uli_key_return;
	state->keymap[RETROK_BACKSPACE] = uli_key_backspace;
	state->keymap[RETROK_DELETE] = uli_key_delete;
	state->keymap[RETROK_INSERT] = uli_key_insert;
	state->keymap[RETROK_PAGEUP] = uli_key_pageup;
	state->keymap[RETROK_PAGEDOWN] = uli_key_pagedown;
	state->keymap[RETROK_HOME] = uli_key_home;
	state->keymap[RETROK_END] = uli_key_end;
	state->keymap[RETROK_UP] = uli_key_up;
	state->keymap[RETROK_DOWN] = uli_key_down;
	state->keymap[RETROK_LEFT] = uli_key_left;
	state->keymap[RETROK_RIGHT] = uli_key_right;
	state->keymap[RETROK_CAPSLOCK] = uli_key_capslock;
	state->keymap[RETROK_LCTRL] = uli_key_ctrl;
	state->keymap[RETROK_RCTRL] = uli_key_ctrl;
	state->keymap[RETROK_LSHIFT] = uli_key_shift;
	state->keymap[RETROK_RSHIFT] = uli_key_shift;
	state->keymap[RETROK_LALT] = uli_key_alt;
	state->keymap[RETROK_RALT] = uli_key_alt;
	state->keymap[RETROK_ESCAPE] = uli_key_escape;
	state->keymap[RETROK_F1] = uli_key_f1;
	state->keymap[RETROK_F2] = uli_key_f2;
	state->keymap[RETROK_F3] = uli_key_f3;
	state->keymap[RETROK_F4] = uli_key_f4;
	state->keymap[RETROK_F5] = uli_key_f5;
	state->keymap[RETROK_F6] = uli_key_f6;
	state->keymap[RETROK_F7] = uli_key_f7;
	state->keymap[RETROK_F8] = uli_key_f8;
	state->keymap[RETROK_F9] = uli_key_f9;
	state->keymap[RETROK_F10] = uli_key_f10;
	state->keymap[RETROK_F11] = uli_key_f11;
	state->keymap[RETROK_F12] = uli_key_f12;
	state->keymap[RETROK_F12] = uli_key_f12;
	state->keymap[RETROK_KP0] = uli_key_numpad0;
	state->keymap[RETROK_KP1] = uli_key_numpad1;
	state->keymap[RETROK_KP2] = uli_key_numpad2;
	state->keymap[RETROK_KP3] = uli_key_numpad3;
	state->keymap[RETROK_KP4] = uli_key_numpad4;
	state->keymap[RETROK_KP5] = uli_key_numpad5;
	state->keymap[RETROK_KP6] = uli_key_numpad6;
	state->keymap[RETROK_KP7] = uli_key_numpad7;
	state->keymap[RETROK_KP8] = uli_key_numpad8;
	state->keymap[RETROK_KP9] = uli_key_numpad9;
	state->keymap[RETROK_KP_PERIOD] = uli_key_numpadperiod;
	state->keymap[RETROK_KP_DIVIDE] = uli_key_numpaddivide;
	state->keymap[RETROK_KP_MULTIPLY] = uli_key_numpadmultiply;
	state->keymap[RETROK_KP_MINUS] = uli_key_numpadminus;
	state->keymap[RETROK_KP_PLUS] = uli_key_numpadplus;
	state->keymap[RETROK_KP_ENTER] = uli_key_numpadenter;
}

/**
 * libretro callback; Global deinitialization.
 */
RETRO_API void retro_deinit(void)
{
	// Make sure the game is unloaded.
	retro_unload_game();

	// Free up the state.
	if (state == NULL) {
		return;
	}

	free(state);
	state = NULL;
}

/**
 * libretro callback; Retrieves the internal libretro API version.
 */
RETRO_API unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

/**
 * libretro callback; Reports device changes.
 */
RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
	log_cb(RETRO_LOG_INFO, "[ULI-78] Plugging device %u into port %u.\n", device, port);
}

/**
 * libretro callback; Retrieves information about the core.
 */
RETRO_API void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name     = ULI_NAME;
	info->library_version  = ULI_VERSION;
	info->valid_extensions = "uli|png";
	info->need_fullpath    = false;
	info->block_extract    = false;
}

/**
 * libretro callback; Get information about the desired audio and video.
 */
RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
	info->timing = (struct retro_system_timing) {
		.fps = ULI78_FRAMERATE,
		.sample_rate = ULI78_SAMPLERATE,
	};

	info->geometry = (struct retro_game_geometry) {
		.base_width   = ULI78_FULLWIDTH,
		.base_height  = ULI78_FULLHEIGHT,
		.max_width    = ULI78_FULLWIDTH,
		.max_height   = ULI78_FULLHEIGHT,
		.aspect_ratio = (float)ULI78_FULLWIDTH / (float)ULI78_FULLHEIGHT,
	};

	if (state->cropBorder) {
		info->geometry.base_width   = ULI78_WIDTH;
		info->geometry.base_height  = ULI78_HEIGHT;
		info->geometry.aspect_ratio = (float)ULI78_WIDTH / (float)ULI78_HEIGHT;
	}
}

/**
 * libretro callback; Sets up the environment callback.
 */
RETRO_API void retro_set_environment(retro_environment_t cb)
{
	// Update the environment callback to make environment calls.
	environ_cb = cb;

	// ULI-78 runner requires a cartridge.
	bool no_content = false;
	cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

	// Set up the logging interface.
	if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
		log_cb = logging.log;
	}
	else {
		log_cb = uli78_libretro_fallback_log;
	}

	// Configure the core settings.
	libretro_set_core_options(environ_cb);
}

/**
 * libretro callback; Set up the audio sample callback.
 */
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_cb = cb;
}

/**
 * libretro callback; Set up the audio sample batch callback.
 *
 * @see uli78_libretro_audio()
 */
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_batch_cb = cb;
}

/**
 * libretro callback; Set up the input poll callback.
 */
RETRO_API void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

/**
 * libretro callback; Set up the input state callback.
 */
RETRO_API void retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

/**
 * libretro callback; Set up the video refresh callback.
 */
RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

/**
 * libretro callback; Reset the game.
 */
RETRO_API void retro_reset(void)
{
	if (state != NULL && state->uli != NULL) {
		uli_mem* uli = (uli_mem*)state->uli;
		uli_api_reset(uli);
	}
}

/**
 * libretro callback; Load the labels for the input buttons.
 *
 * @see uli78_libretro_update()
 */
void uli78_libretro_input_descriptors()
{
	// ULI-78's controller has flipped A/B and X/Y buttons than RetroPad.
	struct retro_input_descriptor desc[] = {

#if ULI_MAXPLAYERS >= 1
		// Player 1
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "A" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "B" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Y" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "X" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Slow Mouse" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Mouse Right Click" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Mouse Left Click" },
#endif

#if ULI_MAXPLAYERS >= 2
		// Player 2
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "A" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "B" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Y" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "X" },
#endif

#if ULI_MAXPLAYERS >= 3
		// Player 3
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "A" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "B" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Y" },
		{ 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "X" },
#endif

#if ULI_MAXPLAYERS >= 4
		// Player 4
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "A" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "B" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Y" },
		{ 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "X" },
#endif

		{ 0 },
	};

	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

/**
 * Retrieve gamepad information from libretro.
 *
 * @see uli78_libretro_update()
 */
void uli78_libretro_update_gamepad(uli78_gamepad* gamepad, uli78_mouse* mouse, int player, bool dpad)
{
	// D-Pad
	if (dpad) {
		gamepad->up = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
		gamepad->down = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
		gamepad->left = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
		gamepad->right = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
	}

	// A/B and X/Y are switched in ULI-78
	gamepad->a = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
	gamepad->b = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
	gamepad->x = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);
	gamepad->y = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);

	// Port 1 shoulder buttons mapped to mouse left/right click/slow mouse
	if (mouse && (player == 0)) {
		mouse->left = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);
		mouse->right = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
		state->slowGamepadMouse = input_state_cb(player, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
	}
}

/**
 * Converts a Pointer API coordinates to screen pixel position.
 *
 * @see uli78_libretro_update_mouse()
 * @see RETRO_DEVICE_POINTER
 */
int uli78_libretro_mouse_pointer_convert(float coord, float full, float margin)
{
	float max         = (float)0x7fff;
	float screenCoord = (((coord + max) / (max * 2.0f) ) * full) - margin;

	// Keep the mouse on the screen.
	if (margin > 0.0f) {
		float limit = full - (margin * 2.0f) - 1.0f;
		screenCoord = (screenCoord < 0.0f)  ? 0.0f  : screenCoord;
		screenCoord = (screenCoord > limit) ? limit : screenCoord;
	}

	return (int)(screenCoord + 0.5f);
}

static void uli78_libretro_update_mouse_wheels(uli78_mouse* mouse)
{
	if (input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP) > 0) {
		mouse->scrollx = 1;
	} else if (input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN) > 0) {
		mouse->scrollx = -1;
	}
	if (input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP) > 0) {
		mouse->scrolly = 1;
	} else if (input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN) > 0) {
		mouse->scrolly = -1;
	}
}

static INLINE float uli78_libretro_get_mouse_delta_physical(unsigned axis)
{
	int delta = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, axis);
	return (float)delta * RETRO_BASE_POINTER_SPEED_PHYSICAL * state->pointerSpeed;
}

static INLINE float uli78_libretro_get_mouse_delta_analog(unsigned index, unsigned axis)
{
	int delta = input_state_cb(0, RETRO_DEVICE_ANALOG, index, axis);
	float delta_amp = 0.0f;

	if ((delta < -state->analogDeadzone) || (delta > state->analogDeadzone)) {
		delta_amp = (float)((delta > state->analogDeadzone) ?
				(delta - state->analogDeadzone) :
						(delta + state->analogDeadzone)) /
								(float)(RETRO_ANALOG_RANGE - state->analogDeadzone);

		delta_amp *= RETRO_BASE_POINTER_SPEED_ANALOG * state->pointerSpeed *
				(state->slowGamepadMouse ? RETRO_SLOW_MOUSE_FACTOR_ANALOG : 1.0f);
	}

	return delta_amp;
}

static INLINE float uli78_libretro_get_mouse_delta_dpad(unsigned axisPlus, unsigned axisMinus)
{
	float delta_amp = 0.0f;

	if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, axisPlus) > 0) {
		delta_amp = RETRO_BASE_POINTER_SPEED_DPAD * state->pointerSpeed *
				(state->slowGamepadMouse ? RETRO_SLOW_MOUSE_FACTOR_DPAD : 1.0f);
	} else if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, axisMinus) > 0) {
		delta_amp = -1.0 * RETRO_BASE_POINTER_SPEED_DPAD * state->pointerSpeed *
				(state->slowGamepadMouse ? RETRO_SLOW_MOUSE_FACTOR_DPAD : 1.0f);
	}

	return delta_amp;
}

/**
 * Retrieve mouse information from libretro.
 */
void uli78_libretro_update_mouse(uli78_mouse* mouse)
{
	mouse->scrollx = 0;
	mouse->scrolly = 0;
	mouse->middle  = 0;

	// Check which device type to poll
	if (state->pointerDevice == POINTER_DEVICE_TOUCHSCREEN) {
		float screenWidth    = (float)ULI78_FULLWIDTH;
		float screenHeight   = (float)ULI78_FULLHEIGHT;
		float screenMarginX  = (float)ULI78_OFFSET_LEFT;
		float screenMarginY  = (float)ULI78_OFFSET_TOP;
		if (state->cropBorder) {
			screenWidth       = (float)ULI78_WIDTH;
			screenHeight      = (float)ULI78_HEIGHT;
			screenMarginX     = 0.0f;
			screenMarginY     = 0.0f;
		}

		// Get the Pointer X and Y, and convert it to screen position
		state->mouseX = uli78_libretro_mouse_pointer_convert(
				input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X),
				screenWidth, screenMarginX);
		state->mouseY = uli78_libretro_mouse_pointer_convert(
				input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y),
				screenHeight, screenMarginY);

		// Pointer pressed is considered mouse left button
		mouse->left = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
		// Touchscreens do not have right or middle buttons,
		// but on Unix at least, the mouse registers as a
		// touchscreen (pointer API) device - so might as
		// well poll the additional mouse buttons
		mouse->right = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
		mouse->middle = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE);
		uli78_libretro_update_mouse_wheels(mouse);
	} else {
		// All other input devices use relative positioning
		float mouseDeltaX = 0;
		float mouseDeltaY = 0;
		int mouseDeltaXInt = 0;
		int mouseDeltaYInt = 0;

		switch (state->pointerDevice) {
			case POINTER_DEVICE_MOUSE:
				// Get Mouse X and Y offsets
				mouseDeltaX = uli78_libretro_get_mouse_delta_physical(RETRO_DEVICE_ID_MOUSE_X);
				mouseDeltaY = uli78_libretro_get_mouse_delta_physical(RETRO_DEVICE_ID_MOUSE_Y);

				// Mouse buttons
				mouse->left = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
				mouse->right = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
				mouse->middle = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE);
				uli78_libretro_update_mouse_wheels(mouse);
			break;
#if ULI_MAXPLAYERS >= 1
			case POINTER_DEVICE_TOUCHSCREEN:
				// Already handled above.
			break;
			case POINTER_DEVICE_LEFT_ANALOG:
				// Get Mouse X and Y offsets
				mouseDeltaX = uli78_libretro_get_mouse_delta_analog(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
				mouseDeltaY = uli78_libretro_get_mouse_delta_analog(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
			break;
			case POINTER_DEVICE_RIGHT_ANALOG:
				// Get Mouse X and Y offsets
				mouseDeltaX = uli78_libretro_get_mouse_delta_analog(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
				mouseDeltaY = uli78_libretro_get_mouse_delta_analog(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
			break;
			case POINTER_DEVICE_DPAD:
				// Get Mouse X and Y offsets
				mouseDeltaX = uli78_libretro_get_mouse_delta_dpad(RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_LEFT);
				mouseDeltaY = uli78_libretro_get_mouse_delta_dpad(RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_UP);
			break;
#endif
		}

		// Determine mouse x/y positions
		if (mouseDeltaX < 0) {
			// Reset accumulator when changing direction,
			// otherwise apply delta
			state->mouseXAccumulator = (state->mouseXAccumulator > 0.0f) ?
					mouseDeltaX : state->mouseXAccumulator + mouseDeltaX;
			// Get integer component of accumulator
			mouseDeltaXInt = (int)state->mouseXAccumulator;
			// Update x position
			mouseDeltaXInt *= -1;
			state->mouseX = (state->mouseX > mouseDeltaXInt) ?
					(state->mouseX - mouseDeltaXInt) : 0;
			// Update accumulator
			state->mouseXAccumulator += (float)mouseDeltaXInt;
		} else {
			// Reset accumulator when changing direction,
			// otherwise apply delta
			state->mouseXAccumulator = (state->mouseXAccumulator < 0.0f) ?
					mouseDeltaX : state->mouseXAccumulator + mouseDeltaX;
			// Get integer component of accumulator
			mouseDeltaXInt = (int)state->mouseXAccumulator;
			// Update x position
			state->mouseX = (state->mouseX + mouseDeltaXInt < ULI78_WIDTH) ?
					(state->mouseX + mouseDeltaXInt) : (ULI78_WIDTH - 1);
			// Update accumulator
			state->mouseXAccumulator -= (float)mouseDeltaXInt;
		}

		if (mouseDeltaY < 0) {
			// Reset accumulator when changing direction,
			// otherwise apply delta
			state->mouseYAccumulator = (state->mouseYAccumulator > 0.0f) ?
					mouseDeltaY : state->mouseYAccumulator + mouseDeltaY;
			// Get integer component of accumulator
			mouseDeltaYInt = (int)state->mouseYAccumulator;
			// Update y position
			mouseDeltaYInt *= -1;
			state->mouseY = (state->mouseY > mouseDeltaYInt) ?
					(state->mouseY - mouseDeltaYInt) : 0;
			// Update accumulator
			state->mouseYAccumulator += (float)mouseDeltaYInt;
		} else {
			// Reset accumulator when changing direction,
			// otherwise apply delta
			state->mouseYAccumulator = (state->mouseYAccumulator < 0.0f) ?
					mouseDeltaY : state->mouseYAccumulator + mouseDeltaY;
			// Get integer component of accumulator
			mouseDeltaYInt = (int)state->mouseYAccumulator;
			// Update y position
			state->mouseY = (state->mouseY + mouseDeltaYInt < ULI78_HEIGHT) ?
					(state->mouseY + mouseDeltaYInt) : (ULI78_HEIGHT - 1);
			// Update accumulator
			state->mouseYAccumulator -= (float)mouseDeltaYInt;
		}
	}

	// Have the mouse disappear after a certain time of inactivity.
	if (state->mouseX != state->mousePreviousX || state->mouseY != state->mousePreviousY) {
		state->mouseHideTimer = state->mouseHideTimerStart;
		state->mousePreviousX = state->mouseX;
		state->mousePreviousY = state->mouseY;
	}
	else if (state->mouseHideTimer > 0) {
		state->mouseHideTimer--;
	}

	// ULI-78 internally offsets the mouse x/y coordinates,
	// so have to adjust libretro values...
	mouse->x = state->mouseX + ULI78_OFFSET_LEFT;
	mouse->y = state->mouseY + ULI78_OFFSET_TOP;
}

/**
 * Gets the 32-bit color value from the ULI-78 palette.
 */
static u32 get_screen_color(uli_mem* uli, u8 index)
{
	uli_rgb color = uli->ram->vram.palette.colors[index];
	// The core requests RETRO_PIXEL_FORMAT_XRGB8888, so we format the color as 0x00RRGGBB.
	return (color.r << 16) | (color.g << 8) | (color.b);
}

/**
 * Draws a single pixel directly to the final screen buffer.
 */
static void draw_pixel_on_screen(u32* screen, s32 x, s32 y, u32 color)
{
	// Bounds check against the visible screen area
	if (x < 0 || x >= ULI78_WIDTH || y < 0 || y >= ULI78_HEIGHT)
		return;

	s32 full_x = x + ULI78_OFFSET_LEFT;
	s32 full_y = y + ULI78_OFFSET_TOP;

	screen[full_y * ULI78_FULLWIDTH + full_x] = color;
}

/**
 * Draws a horizontal line directly to the final screen buffer.
 */
static void draw_hline_on_screen(u32* screen, s32 x1, s32 x2, s32 y, u32 color)
{
	for (s32 x = x1; x <= x2; x++)
		draw_pixel_on_screen(screen, x, y, color);
}

/**
 * Draws a vertical line directly to the final screen buffer.
 */
static void draw_vline_on_screen(u32* screen, s32 x, s32 y1, s32 y2, u32 color)
{
	for (s32 y = y1; y <= y2; y++)
		draw_pixel_on_screen(screen, x, y, color);
}

/**
 * Draws a software cursor on the screen where the mouse is.
 */
void uli78_libretro_mousecursor(uli78* game, uli78_mouse* mouse, enum mouse_cursor_type cursortype)
{
	ULI_UNUSED(mouse);

	// Only draw the mouse cursor if it's active.
	if (state->mouseHideTimerStart > 0 && state->mouseHideTimer == 0) {
		return;
	}

	uli_mem* uli = (uli_mem*)game;
	u32* screen = game->screen;
	s32 mx = state->mouseX;
	s32 my = state->mouseY;

	// Calculate the final 32-bit color value
	u32 cursor_color = get_screen_color(uli, state->mouseCursorColor);

	// Draw the cursor directly to the screen buffer.
	switch (cursortype) {
		case MOUSE_CURSOR_NONE:
			// Nothing.
		break;
		case MOUSE_CURSOR_DOT:
			draw_pixel_on_screen(screen, mx, my, cursor_color);
		break;
		case MOUSE_CURSOR_CROSS:
			draw_hline_on_screen(screen, mx - 4, mx - 2, my, cursor_color);
			draw_hline_on_screen(screen, mx + 2, mx + 4, my, cursor_color);
			draw_vline_on_screen(screen, mx, my - 4, my - 2, cursor_color);
			draw_vline_on_screen(screen, mx, my + 2, my + 4, cursor_color);
		break;
		case MOUSE_CURSOR_ARROW:
		{
			// Calculate black for the outline.
			u32 black_color = get_screen_color(uli, uli_color_black);

			// Draw the filled triangle part of the arrow
			for (int y = 0; y <= 2; y++) {
				for (int x = 0; x <= 2 - y; x++) {
					draw_pixel_on_screen(screen, mx + x, my + y, cursor_color);
				}
			}
			// Draw the black outline (hypotenuse of the triangle)
			draw_pixel_on_screen(screen, mx + 3, my, black_color);
			draw_pixel_on_screen(screen, mx + 2, my + 1, black_color);
			draw_pixel_on_screen(screen, mx + 1, my + 2, black_color);
			draw_pixel_on_screen(screen, mx, my + 3, black_color);
		}
		break;
	}
}

/**
 * Retrieve keyboard information from libretro.
 */
void uli78_libretro_update_keyboard(uli78_keyboard* keyboard)
{
	// Clear the key buffer.
	for (int i = 0; i < ULI78_KEY_BUFFER; i++) {
		keyboard->keys[i] = uli_key_unknown;
	}

	// Load up the active keys into the buffer.
	for (int key = RETROK_FIRST, keyBuffer = 0; key < RETROK_LAST && keyBuffer < ULI78_KEY_BUFFER; key++) {
		if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, key)) {
			keyboard->keys[keyBuffer++] = state->keymap[key];
		}
	}
}

/**
 * Update the input state, and tick the game.
 */
void uli78_libretro_update(uli78* game)
{
	// Let libretro know that we need updated input states.
	input_poll_cb();

	// Gamepads
#if ULI_MAXPLAYERS >= 1
	uli78_libretro_update_gamepad(&state->input.gamepads.first,
			&state->input.mouse, 0, state->pointerDevice != POINTER_DEVICE_DPAD);
#endif

#if ULI_MAXPLAYERS >= 2
	uli78_libretro_update_gamepad(&state->input.gamepads.second, NULL, 1, true);
#endif

#if ULI_MAXPLAYERS >= 3
	uli78_libretro_update_gamepad(&state->input.gamepads.third, NULL, 2, true);
#endif

#if ULI_MAXPLAYERS >= 4
	uli78_libretro_update_gamepad(&state->input.gamepads.fourth, NULL, 3, true);
#endif

	// Mouse
	uli78_libretro_update_mouse(&state->input.mouse);

	// Keyboard
	uli78_libretro_update_keyboard(&state->input.keyboard);

	// Update the game state.
	uli78_tick(game, state->input, uli78_libretro_counter, uli78_libretro_frequency);
	uli78_sound(game);
}

/**
 * Draw the screen.
 */
void uli78_libretro_draw(uli78* game)
{
	// Render the mouse cursor if needed.
	uli78_libretro_mousecursor((uli78*)game, &state->input.mouse, state->mouseCursor);

	// Render to the screen.
	if (state->cropBorder) {
		u32 *screen = (u32*)game->screen + (ULI78_FULLWIDTH * ULI78_OFFSET_TOP) + ULI78_OFFSET_LEFT;
		video_cb(screen, ULI78_WIDTH, ULI78_HEIGHT, ULI78_FULLWIDTH << 2);
	} else {
		video_cb(game->screen, ULI78_FULLWIDTH, ULI78_FULLHEIGHT, ULI78_FULLWIDTH << 2);
	}
}

/**
 * Play the ULI-78 audio.
 *
 * @see retro_run()
 */
void uli78_libretro_audio(uli78* game)
{
	// Tell libretro about the samples.
	audio_batch_cb(game->samples.buffer, game->samples.count / ULI78_SAMPLE_CHANNELS);
}

/**
 * Update the state of the core variables.
 *
 * @see retro_run()
 */
void uli78_libretro_variables(bool startup)
{
	// Check all the individual variables for the core.
	struct retro_variable var;
	bool lastCropBorder = state->cropBorder;

	// Crop Border
	state->cropBorder = false;
	var.key = "uli78_crop_border";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
		if (strcmp(var.value, "enabled") == 0) {
			state->cropBorder = true;
		}
	}

	if (!startup && (state->cropBorder != lastCropBorder)) {
		struct retro_system_av_info av_info;
		retro_get_system_av_info(&av_info);
		environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av_info);
	}

	// Pointer device
	state->pointerDevice = POINTER_DEVICE_MOUSE;
	var.key = "uli78_pointer_device";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
		if (strcmp(var.value, "touchscreen") == 0) {
			state->pointerDevice = POINTER_DEVICE_TOUCHSCREEN;
		}
#if ULI_MAXPLAYERS >= 1
		else if (strcmp(var.value, "left_analog") == 0) {
			state->pointerDevice = POINTER_DEVICE_LEFT_ANALOG;
		}
		else if (strcmp(var.value, "right_analog") == 0) {
			state->pointerDevice = POINTER_DEVICE_RIGHT_ANALOG;
		}
		else if (strcmp(var.value, "dpad") == 0) {
			state->pointerDevice = POINTER_DEVICE_DPAD;
		}
#endif
	}

	// Pointer Speed
	state->pointerSpeed = 1.0f;
	var.key = "uli78_pointer_speed";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
		state->pointerSpeed = (float)atoi(var.value) * 0.01f;
	}

	// Mouse Cursor
	state->mouseCursor = MOUSE_CURSOR_NONE;
	var.key = "uli78_mouse_cursor";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
		if (strcmp(var.value, "dot") == 0) {
			state->mouseCursor = MOUSE_CURSOR_DOT;
		}
		else if (strcmp(var.value, "cross") == 0) {
			state->mouseCursor = MOUSE_CURSOR_CROSS;
		}
		else if (strcmp(var.value, "arrow") == 0) {
			state->mouseCursor = MOUSE_CURSOR_ARROW;
		}
	}

	// Mouse Cursor Color
	state->mouseCursorColor = 15;
	var.key = "uli78_mouse_cursor_color";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
		state->mouseCursorColor = (u8)atoi(var.value);
	}

	// Mouse Hide Delay
	state->mouseHideTimerStart = 0;
	var.key = "uli78_mouse_hide_delay";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
		state->mouseHideTimerStart = atoi(var.value);
		if (state->mouseHideTimerStart > 0) {
			state->mouseHideTimerStart = state->mouseHideTimerStart * ULI78_FRAMERATE;
			state->mouseHideTimer = 0; // Cursor starts hidden
		}
		else {
			state->mouseHideTimerStart = 0;
			state->mouseHideTimer = 0;
		}
	}

	// Gamepad Analog Deadzone
	state->analogDeadzone = (int)(0.15f * (float)RETRO_ANALOG_RANGE);
	var.key = "uli78_analog_deadzone";
	var.value = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
		state->analogDeadzone = (int)((float)atoi(var.value) * 0.01f * (float)RETRO_ANALOG_RANGE);
	}
}

/**
 * libretro callback; Render the screen and play the audio.
 */
RETRO_API void retro_run(void)
{
	// Ensure the state is set up.
	if (state == NULL || state->uli == NULL) {
		return;
	}

	// Update the ULI-78 environment.
	uli78_libretro_update(state->uli);

	// Check if the game requested to quit.
	if (state->quit) {
		retro_deinit();
		environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
		return;
	}

	// Render the screen.
	uli78_libretro_draw(state->uli);

	// Play the audio.
	uli78_libretro_audio(state->uli);

	// Update core options, if needed.
	bool updated = false;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
		uli78_libretro_variables(false);
	}
}

/**
 * libretro callback; Load a game.
 */
RETRO_API bool retro_load_game(const struct retro_game_info *info)
{
	// TODO: Warn that Audio Synchronization required to run at a proper speed.
	// TODO: Warn that the core doesn't support Runahead.

	// Initialize the core if it hasn't been yet.
	if (state == NULL) {
		retro_init();
	}

	// Pixel format.
	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
		log_cb(RETRO_LOG_ERROR, "[ULI-78] RETRO_PIXEL_FORMAT_XRGB8888 is not supported.\n");
		return false;
	}

	// Check for the content.
	if (info == NULL) {
		log_cb(RETRO_LOG_ERROR, "[ULI-78] No content information provided.\n");
		return false;
	}

	// Ensure content data is available.
	if (info->data == NULL) {
		log_cb(RETRO_LOG_ERROR, "[ULI-78] No content data provided.\n");
		return false;
	}

	// Set up the frame time callback.
	struct retro_frame_time_callback frame_time = {
		.callback = uli78_libretro_frame_time,
		.reference = ULI78_FREQUENCY / ULI78_FRAMERATE,
	};
	if (!environ_cb(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame_time)) {
		log_cb(RETRO_LOG_ERROR, "[ULI-78] Failed to set frame time callback.\n");
		return false;
	}

	// Set up the ULI-78 environment.
#if RETRO_IS_BIG_ENDIAN
	state->uli = uli78_create(ULI78_SAMPLERATE, ULI78_PIXEL_COLOR_ARGB8888);
#else
	state->uli = uli78_create(ULI78_SAMPLERATE, ULI78_PIXEL_COLOR_BGRA8888);
#endif
	if (state->uli == NULL) {
		log_cb(RETRO_LOG_ERROR, "[ULI-78] Failed to initialize ULI-78 environment.\n");
		return false;
	}

	// Set up the environment variables.
	state->uli->callback.exit = uli78_libretro_exit;
	state->uli->callback.error = uli78_libretro_error;
	state->uli->callback.trace = uli78_libretro_trace;

	// Initialize some of the game state.
	state->quit = false;
	state->input.mouse.x = 0;
	state->input.mouse.y = 0;

	// Load the content.
	// TODO: Allow loading code files directly.
	uli78_load(state->uli, (void*)(info->data), (int)info->size);
	if (state->uli == NULL) {
		log_cb(RETRO_LOG_ERROR, "[ULI-78] Content loaded, but failed to load game.\n");
		retro_unload_game();
		return false;
	}

	// Set up the input descriptors.
	uli78_libretro_input_descriptors();

	// Load up any core variables.
	uli78_libretro_variables(true);

	return true;
}

/**
 * libretro callback; Tells the core to unload the game.
 */
RETRO_API void retro_unload_game(void)
{
	if (state == NULL || state->uli == NULL) {
		return;
	}

	uli78_delete(state->uli);
	state->uli = NULL;
}

/**
 * libretro callback; Retrieves the region for the content.
 */
RETRO_API unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

/**
 * libretro callback; Load a game using a subsystem.
 */
RETRO_API bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	ULI_UNUSED(num);
	ULI_UNUSED(type);
	// Forward subsystem requests over to retro_load_game().
	return retro_load_game(info);
}

/**
 * libretro callback; Retrieve the size of the serialized memory.
 */
size_t retro_serialize_size(void)
{
	return ULI_PERSISTENT_SIZE * sizeof(u32);
}

/**
 * libretro callback; Get the current persistent memory.
 */
RETRO_API bool retro_serialize(void *data, size_t size)
{
	ULI_UNUSED(size);
	if (state == NULL || state->uli == NULL || data == NULL) {
		return false;
	}

	uli_mem* uli = (uli_mem*)state->uli;
	u32* udata = (u32*)data;
	for (u32 i = 0; i < ULI_PERSISTENT_SIZE; i++) {
		udata[i] = uli->ram->persistent.data[i];
	}

	return true;
}

/**
 * libretro callback; Given the serialized data, load it into the persistent memory.
 */
RETRO_API bool retro_unserialize(const void *data, size_t size)
{
	if (state == NULL || state->uli == NULL || size != retro_serialize_size() || data == NULL) {
		return false;
	}

	uli_mem* uli = (uli_mem*)state->uli;
	u32* uData = (u32*)data;
	for (u32 i = 0; i < ULI_PERSISTENT_SIZE; i++) {
		uli->ram->persistent.data[i] = uData[i];
	}

	return true;
}

/**
 * libretro callback; Gets region of memory.
 *
 * https://github.com/uli78/ULI-78/wiki/ram
 */
RETRO_API void *retro_get_memory_data(unsigned id)
{
	if (state == NULL || state->uli == NULL) {
		return NULL;
	}

	uli_mem* uli = (uli_mem*)state->uli;
	switch (id) {
		case RETRO_MEMORY_SAVE_RAM:
			return uli->ram->persistent.data;
		case RETRO_MEMORY_SYSTEM_RAM:
			return uli->ram->data;
		case RETRO_MEMORY_VIDEO_RAM:
			return uli->ram->vram.data;
		default:
			return NULL;
	}
}

/**
 * libretro callback; Gets the size of the given memory slot.
 */
RETRO_API size_t retro_get_memory_size(unsigned id)
{
    if (state == NULL || state->uli == NULL) {
        return 0;
    }

    uli_mem* uli = (uli_mem*)state->uli;
    switch (id) {
        case RETRO_MEMORY_SAVE_RAM:
            return sizeof(uli->ram->persistent.data);
        case RETRO_MEMORY_SYSTEM_RAM:
            return sizeof(uli->ram->data);
        case RETRO_MEMORY_VIDEO_RAM:
            return sizeof(uli->ram->vram.data);
        default:
            return 0;
    }
}

/**
 * libretro callback; Reset all cheats to disabled.
 */
RETRO_API void retro_cheat_reset(void)
{
	// Nothing.
}

/**
 * libretro callback; Enable/disable the given cheat code.
 *
 * ULI-78 codes expect a space-seperated list of paired
 * integers. The first number is the index in pmem(), the
 * second value is the value for the pmem() call. Example:
 *
 * ```
 * cheats = 1
 *
 * cheat0_desc = "3 Lives"
 * cheat0_code = "255 3"
 * cheat0_enable = false
 * ```
 *
 * The above cheat would be the same as calling:
 *
 * pmem(255, 3)
 *
 * @see https://github.com/uli78/ULI-78/wiki/pmem
 */
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	ULI_UNUSED(index);
	ULI_UNUSED(enabled);
	if (!state || !state->uli) {
		return;
	}

	u32 codes[ULI_PERSISTENT_SIZE];
	u32 codeIndex = 0;
	char *str = (char*)code;
	char *end = str;

	// Split the code by spaces, to get an array of integers.
	while (*end && codeIndex < ULI_PERSISTENT_SIZE) {
		codes[codeIndex++] = strtol(str, &end, 10);
		while (*end == ' ') {
			end++;
		}
		str = end;
	}

	// Finally, set each given code pair.
	uli_mem* uli = (uli_mem*)state->uli;
	for (u32 i = 0; i < codeIndex; i = i + 2) {
		uli->ram->persistent.data[codes[i]] = codes[i+1];
	}
}
