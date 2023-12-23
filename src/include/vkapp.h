#include <vulkan/vulkan.h>
#include "SDL.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    char* title;
    SDL_Window* window;
    VkInstance instance;
} VkApp;

void app_createVulkanInstance(VkApp* pApp) {
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = pApp->title,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags = 0,
        .pNext = NULL,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL
    };

    // get extension count first
    uint32_t sdlExtensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(pApp->window, &sdlExtensionCount, NULL)) {
        fprintf(stderr, "ERROR: Failed to get instance extensions count. %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    printf("SDL extension amount: %d\n", sdlExtensionCount);
    // then allocate and get the extension array
    const char** sdlExtensions = malloc(sdlExtensionCount * sizeof(const char*));
    if (!SDL_Vulkan_GetInstanceExtensions(pApp->window, &sdlExtensionCount, sdlExtensions)) {
        fprintf(stderr, "ERROR: Failed to get instance extensions count. %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    createInfo.enabledExtensionCount = sdlExtensionCount;
    createInfo.ppEnabledExtensionNames = sdlExtensions;

    printf("heyo, before the vkCreateInstance!\n");
    VkResult result = vkCreateInstance(&createInfo, NULL, &pApp->instance);
    printf("heyo, after the vkCreateInstance!\n");
    assert(result == VK_SUCCESS);
    free(sdlExtensions);
}

void app_initVulkanSDL(VkApp* pApp) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    pApp->window = SDL_CreateWindow(pApp->title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, pApp->width, pApp->height, SDL_WINDOW_VULKAN);
    if (!pApp->window) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    app_createVulkanInstance(pApp);
}

void app_render(VkApp* pApp) {
    return;
}

void app_mainLoop(VkApp* pApp) {
    SDL_Event event;
    while (1) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        app_render(pApp);
    }
}

void app_cleanup(VkApp* pApp) {
    vkDestroyInstance(pApp->instance, NULL);
    SDL_DestroyWindow(pApp->window);
    SDL_Quit();
}

int app_run(VkApp* pApp) {
    app_initVulkanSDL(pApp);
    app_mainLoop(pApp);
    app_cleanup(pApp);
    return 0;
}
