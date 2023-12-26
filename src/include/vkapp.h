#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"

#include "vkapp_types.h"
#include "vkapp_debug.h"
#include "vkapp_vulkan.h"

void app_initSDLWindow(VkApp *pApp) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        exit(1);
    }

    pApp->window = SDL_CreateWindow(pApp->title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, pApp->width, pApp->height, SDL_WINDOW_VULKAN);
    if (!pApp->window) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetWindowResizable(pApp->window, SDL_FALSE);
}

void app_initVulkan(VkApp *pApp) {
    app_createVulkanInstance(pApp);
    setupDebugMessenger(pApp);
    createSurface(pApp);
    pickPhysicalDevice(pApp);
    createLogicalDevice(pApp);
    createSwapChain(pApp);
    createImageViews(pApp);
    createRenderPass(pApp);
    createGraphicsPipeline(pApp);
    createFramebuffers(pApp);
    createCommandPool(pApp);
    createCommandBuffer(pApp);
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
    vkDestroyCommandPool(pApp->device, pApp->commandPool, NULL);
    vkDestroyPipeline(pApp->device, pApp->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(pApp->device, pApp->pipelineLayout, NULL);
    vkDestroyRenderPass(pApp->device, pApp->renderPass, NULL);

    for (uint32_t i = 0; i < pApp->swapChainImageCount; i++) {
        vkDestroyImageView(pApp->device, pApp->swapChainImageViews[i], NULL);
        vkDestroyFramebuffer(pApp->device, pApp->swapChainFramebuffers[i], NULL);
    }
    free(pApp->swapChainFramebuffers);
    free(pApp->swapChainImages);
    free(pApp->swapChainImageViews);
    vkDestroySwapchainKHR(pApp->device, pApp->swapChain, NULL);
    vkDestroyDevice(pApp->device, NULL);
    vkDestroySurfaceKHR(pApp->instance, pApp->surface, NULL);
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
