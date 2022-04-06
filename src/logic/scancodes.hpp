#pragma once

#include <grend/sdlContext.hpp>
#include <logic/gameController.hpp>
// JAM HAX: TODO: move this to a .cpp file

inline uint8_t foo[SDL_NUM_SCANCODES];
inline uint8_t bar[SDL_NUM_SCANCODES];
inline uint8_t edgefoo[SDL_NUM_SCANCODES];
inline uint8_t edgebar[SDL_NUM_SCANCODES];

inline bool scancodePressed(int n) {
	const uint8_t *keystates = SDL_GetKeyboardState(NULL);
	return keystates[n];
}

inline bool toggleScancode(int n) {
	const uint8_t *keystates = SDL_GetKeyboardState(NULL);

	if (keystates[n] && !foo[n]) {
		foo[n] = 1;
		return true;

	} else if (!keystates[n] && foo[n]) {
		foo[n] = 0;
		return false;
	}

	return false;
}

inline bool edgeScancode(int n) {
	const uint8_t *keystates = SDL_GetKeyboardState(NULL);

	return keystates[n] && edgefoo[n];
}

inline bool toggleGamepad(SDL_GameControllerButton n) {
	if (!Controller()) return false;

	bool pressed = SDL_GameControllerGetButton(Controller(), n);

	if (pressed && !bar[n]) {
		bar[n] = 1;
		return true;
	}

	else if (!pressed && bar[n]) {
		bar[n] = 0;
		return false;
	}

	return false;
}

inline bool edgeGamepad(SDL_GameControllerButton n) {
	if (!Controller()) return false;
	bool pressed = SDL_GameControllerGetButton(Controller(), n);

	return pressed && edgebar[n];
}
