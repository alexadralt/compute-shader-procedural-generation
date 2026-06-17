#pragma once

#include <SDL3/SDL.h>

class App {
	SDL_Window* window;

	void quit();

public:
	App() : window(nullptr) {};
	~App() { quit(); };

	App(const App& other) = delete;
	App(App&& other) = delete;

	App& operator=(const App& other) = delete;
	App& operator=(App&& other) = delete;

	bool init();
	void main_loop();
};
