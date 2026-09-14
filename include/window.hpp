#pragma once

#include "SDL3/SDL.h"
#include "SDL3/SDL_gpu.h"

#include <cstdint>
#include <string>

namespace how2sdl {
    void init();
    void quit();

    class window {
    public:

        window(uint32_t width, uint32_t height, std::string title, bool debug = false);
        ~window();

        window(const window&) = delete;
        window& operator=(const window&) = delete;

        window(window&& other) noexcept;
        window& operator=(window&& other) noexcept;

        SDL_Window* SDLWindow = nullptr;
        SDL_GPUDevice* device = nullptr;

        uint32_t width = 0;
        uint32_t height = 0;
        std::string title = "";
        bool debug = false;

    private:

        void release();

    };
}