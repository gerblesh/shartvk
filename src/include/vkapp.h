#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <vulkan/vulkan.h>
#include "SDL.h"
#include "vkapp_types.h"
#include "vkapp_debug.h"
#include "vkapp_vulkan.h"

void app_initSDLWindow(VkApp* pApp) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        exit(1);
    }

    pApp->window = SDL_CreateWindow(pApp->title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, pApp->width, pApp->height, SDL_WINDOW_VULKAN);
    if (!pApp->window) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        exit(1);
    }
}

void app_initVulkan(VkApp *pApp) {
    app_createVulkanInstance(pApp);
    setupDebugMessenger(pApp);
    pickPhysicalDevice(pApp);
}

void app_mainLoop(VkApp *pApp) {
    SDL_Event event;
    while (true) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        app_render(pApp);
    }
}

void app_cleanup(VkApp *pApp) {
    if (ENABLE_VALIDATION_LAYERS) {
        DestroyDebugUtilsMessengerEXT(pApp->instance, pApp->debugMessenger, NULL);
    }
    vkDestroyInstance(pApp->instance, NULL);
    
    SDL_DestroyWindow(pApp->window);
    SDL_Quit();
}

int app_run(VkApp *pApp) {
    app_initSDLWindow(pApp);
    app_initVulkan(pApp);
    app_mainLoop(pApp);
    app_cleanup(pApp);
    return 0;
}
