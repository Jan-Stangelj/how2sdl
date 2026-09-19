#include "window.hpp"

#include "SDL3_shadercross/SDL_shadercross.h"

#include <iostream>

namespace how2sdl {
    void init() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "Failed to init SDL: " << SDL_GetError() << "\n";
        }
        if (!SDL_ShaderCross_Init()) {
            std::cerr << "Failed to init SDL shadercross\n";
        }
    }

    void quit() {
        SDL_ShaderCross_Quit();
        SDL_Quit();
    }

    window::window(uint32_t width, uint32_t height, std::string title, bool debug) : width(width),
                                                                                     height(height),
                                                                                     title(title),
                                                                                     debug(debug) {
                                                                                        
        SDLWindow = SDL_CreateWindow(title.c_str(), width, height, 0);
        if (SDLWindow == nullptr) {
            std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        }

        device = SDL_CreateGPUDevice(SDL_ShaderCross_GetSPIRVShaderFormats(), debug, NULL);
        if (device == nullptr) {
            std::cerr << "Failed to create GPU device: " << SDL_GetError() << '\n';
            release();
        }

        if (!SDL_ClaimWindowForGPUDevice(device, SDLWindow)) {
            std::cerr << "Failed to claim window for GPU device: " << SDL_GetError() << '\n';
            release();
        }
    }

    window::~window() {
        release();
    }

    window::window(window&& other) noexcept
        : SDLWindow(other.SDLWindow), device(other.device),
        width(other.width), height(other.height),
        title(std::move(other.title)), debug(other.debug) {
        other.SDLWindow = nullptr;
        other.device = nullptr;
    }

    window& window::operator=(window&& other) noexcept {
        if (this != &other) {
            release();
            SDLWindow = other.SDLWindow;
            device = other.device;
            width = other.width;
            height = other.height;
            title = std::move(other.title);
            debug = other.debug;
            other.SDLWindow = nullptr;
            other.device = nullptr;
        }
        return *this;
    }

    void window::release() {
        if (SDLWindow != nullptr && device != nullptr) SDL_ReleaseWindowFromGPUDevice(device, SDLWindow);
        if (device != nullptr) SDL_DestroyGPUDevice(device);
        if (SDLWindow != nullptr) SDL_DestroyWindow(SDLWindow);

        SDLWindow = nullptr;
        device = nullptr;
    }
}