#include "SDL3/SDL.h"
#include "SDL3/SDL_gpu.h"

#include <cstdint>
#include <iostream>

struct vertex {
    float position[4];
    float color[4];
};

SDL_GPUShader* LoadShader(
    SDL_GPUDevice *device,
    const char *filename,
    SDL_GPUShaderStage stage,
    Uint32 num_samplers,
    Uint32 num_storage_textures,
    Uint32 num_storage_buffers,
    Uint32 num_uniform_buffers) {

    size_t size = 0;

    void *code = SDL_LoadFile(filename, &size);
    if (code == NULL) {
        std::cerr << "Failed to read shader from file: " << SDL_GetError() << '\n';
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
    if (shader == NULL) {
        std::cerr << "Failed to create GPU shader: " << SDL_GetError() << '\n';
        SDL_free(code);
        return nullptr;
    }

    SDL_free(code);

    return shader;
}

int main(void) {

    //============|
    // Init begin |
    //============|

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to init SDL: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("how2sdl", 800, 600, 0);
    if (window == NULL) {
        std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
    if (device == nullptr) {
        std::cerr << "Failed to create GPU device: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        std::cerr << "Failed to claim window for GPU device: " << SDL_GetError() << '\n';
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    //===============|
    // Init end      |
    // Buffers begin |
    //===============|

    // buffer data

    vertex vertices[] = {
        // Bottom-left
        {
            { -0.7f, -0.7f, 0.0f, 1.0f },
            {  1.0f,  0.0f, 0.0f, 1.0f }
        },

        // Bottom-right
        {
            {  0.7f, -0.7f, 0.0f, 1.0f },
            {  0.0f,  1.0f, 0.0f, 1.0f }
        },

        // Top-right
        {
            {  0.7f,  0.7f, 0.0f, 1.0f },
            {  0.0f,  0.0f, 1.0f, 1.0f }
        },

        // Top-left
        {
            { -0.7f,  0.7f, 0.0f, 1.0f },
            {  1.0f,  1.0f, 0.0f, 1.0f }
        }
    };

    static const uint32_t indices[] = {
        0, 1, 2,
        2, 0, 3
    };

    // create buffers

    SDL_GPUBufferCreateInfo vertexBufferInfo{};
    vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    vertexBufferInfo.size = sizeof(vertices);

    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(device, &vertexBufferInfo);
    if (vertexBuffer == NULL) {
        std::cerr << "Failed to create vertex GPU buffer: " << SDL_GetError() << '\n';
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GPUBufferCreateInfo indexBufferInfo{};
    indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    indexBufferInfo.size = sizeof(indices);

    SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(device, &indexBufferInfo);
    if (indexBuffer == NULL) {
        std::cerr << "Failed to create index GPU buffer: " << SDL_GetError() << '\n';
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // copy data to transfer buffer

    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = sizeof(vertices) + sizeof(indices);

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (transferBuffer == NULL) {
        std::cerr << "Failed to create GPU transfer buffer: " << SDL_GetError() << '\n';
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    void* data = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    if (data == NULL) {
        std::cerr << "Failed to map GPU transfer buffer: " << SDL_GetError() << '\n';
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_memcpy(data, vertices, sizeof(vertices));
    SDL_memcpy((uint8_t*)data + sizeof(vertices), indices, sizeof(indices));

    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // upload from transfer buffer to the gpu

    SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
    if (uploadCmd == NULL) {
        std::cerr << "Failed to acquire gpu upload command buffer: " << SDL_GetError() << '\n';
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

    // vertex
    SDL_GPUTransferBufferLocation vertexSource{};
    vertexSource.transfer_buffer = transferBuffer;
    vertexSource.offset = 0;

    SDL_GPUBufferRegion vertexDestination{};
    vertexDestination.buffer = vertexBuffer;
    vertexDestination.offset = 0;
    vertexDestination.size = sizeof(vertices);

    SDL_UploadToGPUBuffer(copyPass, &vertexSource, &vertexDestination, false);

    // index
    SDL_GPUTransferBufferLocation indexSource{};
    indexSource.transfer_buffer = transferBuffer;
    indexSource.offset = sizeof(vertices);

    SDL_GPUBufferRegion indexDestination{};
    indexDestination.buffer = indexBuffer;
    indexDestination.offset = 0;
    indexDestination.size = sizeof(indices);

    SDL_UploadToGPUBuffer(copyPass, &indexSource, &indexDestination, false);

    SDL_EndGPUCopyPass(copyPass);
    if (!SDL_SubmitGPUCommandBuffer(uploadCmd)) {
        std::cerr << "Failed to submit upload GPU command buffer: " << SDL_GetError() << '\n';
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (!SDL_WaitForGPUIdle(device))
        std::cerr << "Failed to wait for GPU idle: " << SDL_GetError() << '\n';
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    //================|
    // Buffers end    |
    // Pipeline begin |
    //================|

    SDL_GPUShader* vert = LoadShader(device, "shaders/triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 2, 0);
    SDL_GPUShader* frag = LoadShader(device, "shaders/triangle.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);

    if (vert == nullptr || frag == nullptr) {
        std::cerr << "Failed to load shaders\n";

        if (vert) SDL_ReleaseGPUShader(device, vert);
        if (frag) SDL_ReleaseGPUShader(device, frag);

        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);

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

    if (pipeline == NULL) {
        std::cerr << "Failed to create graphics pipeline: " << SDL_GetError() << '\n';
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    //===================|
    // Pipeline end      |
    // Render loop begin |
    //===================|

    bool running = true;
    int exitCode = 0;

    while (running) {

        //=============|
        // Input begin |
        //=============|

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        //==========================|
        // Input end                |
        // Get command buffer begin |
        //==========================|

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        if (cmd == nullptr) {
            std::cerr << "Failed to acquire gpu command buffer: " << SDL_GetError() << '\n';
            exitCode = 1;
            break;
        }

        //=============================|
        // Get command buffer end      | 
        // Get swapchain texture begin |
        //=============================|

        SDL_GPUTexture* swapchainTexture = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;

        if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTexture, &width, &height)) {
            std::cerr << "Failed to acquire gpu swapchain texture: " << SDL_GetError() << '\n';
            SDL_CancelGPUCommandBuffer(cmd);
            exitCode = 1;
            break;
        }
        if (swapchainTexture == NULL) {
            SDL_CancelGPUCommandBuffer(cmd);
            continue;
        }

        //===========================|
        // Get swapchain texture end |
        // Render pass begin         |
        //===========================|

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

        // bind the vertex and index ssbo
        SDL_GPUBuffer* storageBuffers[] = { vertexBuffer, indexBuffer };
        SDL_BindGPUVertexStorageBuffers(pass, 0, storageBuffers, 2);

        SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);

        SDL_EndGPURenderPass(pass);

        //=================|
        // Render pass end |
        //=================|

        // Submit the commands to the gpu
        if (!SDL_SubmitGPUCommandBuffer(cmd)) {
            std::cerr << "Failed to submit gpu command buffer: " << SDL_GetError() << '\n';
            exitCode = 1;
            break;
        }
    }

    //=================|
    // Render loop end |
    // Cleanup begin   |
    //=================|

    SDL_WaitForGPUIdle(device);

    SDL_ReleaseGPUBuffer(device, vertexBuffer);
    SDL_ReleaseGPUBuffer(device, indexBuffer);

    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_DestroyGPUDevice(device);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return exitCode;
}