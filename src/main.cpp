#include "SDL3/SDL.h"
#include "SDL3/SDL_gpu.h"

#include <iostream>

static SDL_GPUShader *LoadShader(
    SDL_GPUDevice *device,
    const char *filename,
    SDL_GPUShaderStage stage,
    Uint32 num_samplers,
    Uint32 num_storage_textures,
    Uint32 num_storage_buffers,
    Uint32 num_uniform_buffers
)
{
    size_t size = 0;

    void *code = SDL_LoadFile(filename, &size);
    if (!code) {
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info{};
    info.code = (const Uint8 *)code;
    info.code_size = size;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage = stage;

    info.num_samplers = num_samplers;
    info.num_storage_textures = num_storage_textures;
    info.num_storage_buffers = num_storage_buffers;
    info.num_uniform_buffers = num_uniform_buffers;

    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);

    SDL_free(code);

    return shader;
}

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to init SDL\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("how2sdl", 800, 600, 0);
    if (window == nullptr) {
        std::cerr << "Failed to create window\n";
        SDL_Quit();
        return 1;
    }

    SDL_GPUDevice* device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV,
        true,
        "vulkan"
    );
    if (device == nullptr) {
        std::cerr << "Failed to create gpu device\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GPUShader* vert = LoadShader(device, "shaders/triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 0, 0);
    SDL_GPUShader* frag = LoadShader(device, "shaders/triangle.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);

    if (vert == nullptr || frag == nullptr) {
        std::cerr << "Failed to load shaders\n";

        if (vert) SDL_ReleaseGPUShader(device, vert);
        if (frag) SDL_ReleaseGPUShader(device, frag);

        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GPUColorTargetDescription colorTargetDescription{};
    colorTargetDescription.format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};

    pipelineInfo.vertex_shader = vert;
    pipelineInfo.fragment_shader = frag;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTargetDescription;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

    SDL_ReleaseGPUShader(device, vert);
    SDL_ReleaseGPUShader(device, frag);

    if (pipeline == nullptr) {
        std::cerr << "Failed to create graphics pipeline\n";

        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        if (cmd == nullptr) {
            std::cerr << "Failed to acquire gpu command buffer\n";
            break;
        }

        SDL_GPUTexture* swapchainTexture = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;

        if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTexture, &width, &height)) {
            std::cerr << "Failed to acquire gpu swapchain texture\n";
            SDL_CancelGPUCommandBuffer(cmd);
            break;
        }

        if (swapchainTexture == nullptr) {
            std::cerr << "Swapchain texture dosen't exist\n";
            SDL_CancelGPUCommandBuffer(cmd);
            continue;
        }

        SDL_GPUColorTargetInfo colorTarget{};

        colorTarget.texture = swapchainTexture;

        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        colorTarget.clear_color.r = 0.0f;
        colorTarget.clear_color.g = 0.0f;
        colorTarget.clear_color.b = 0.0f;
        colorTarget.clear_color.a = 1.0f;

        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);

        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(pass);

        if (!SDL_SubmitGPUCommandBuffer(cmd)) {
            std::cerr << "Failed to submit gpu command buffer\n";

            break;
        }
    }

    SDL_WaitForGPUIdle(device);

    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_DestroyGPUDevice(device);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}