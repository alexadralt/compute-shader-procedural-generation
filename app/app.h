#pragma once

#include "renderer/device.h"

#include <SDL3/SDL.h>

class App {
    SDL_Window* m_window;
    rdr::Device m_rdr_device;

    void quit();

public:
    App() : m_window(nullptr),
            m_rdr_device() {};
    ~App() { quit(); };

    App(const App& other) = delete;
    App(App&& other) = delete;

    App& operator=(const App& other) = delete;
    App& operator=(App&& other) = delete;

    bool init();
    void main_loop();
};
