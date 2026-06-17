#include "app.h"

#include <print>

void App::quit()
{
	if (window) {
		SDL_DestroyWindow(window);
	}
}

bool App::init()
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::println("Failed to initialize sdl: {}", SDL_GetError());
		return false;
	}

	window = SDL_CreateWindow("Hello motherfucker!", 1920, 1080, SDL_WINDOW_RESIZABLE);
	if (!window) {
		std::println("Failed to create window: {}", SDL_GetError());
		return false;
	}

	return true;
}

void App::main_loop()
{
	bool running = true;
	while (running) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
				{
					running = false;
				} break;
			}
		}
	}
}
