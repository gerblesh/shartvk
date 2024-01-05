#include <stdint.h>
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
    SDL_SetWindowResizable(pApp->window, SDL_TRUE);
}

void app_initVulkan(VkApp *pApp) {
    loadModel();
    app_createVulkanInstance(pApp);
    setupDebugMessenger(pApp);
    createSurface(pApp);
    pickPhysicalDevice(pApp);
    createLogicalDevice(pApp);
    createSwapChain(pApp);
    createImageViews(pApp);
    createRenderPass(pApp);
    createDescriptorSetLayout(pApp);
    createGraphicsPipeline(pApp);
    createCommandPool(pApp);
    createDepthResources(pApp);
    createFramebuffers(pApp);
    createTextureImage(pApp);
    createTextureImageView(pApp);
    createTextureSampler(pApp);
    createVertexBuffer(pApp);
    createIndexBuffer(pApp);
    createUniformBuffers(pApp);
    createDescriptorPool(pApp);
    createDescriptorSets(pApp);
    createCommandBuffers(pApp);
    createSyncObjects(pApp);
}

void app_mainLoop(VkApp *pApp) {
    SDL_Event event;
    while (true) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        if (event.type == SDL_WINDOWEVENT) {
            // recreateSwapChain(pApp);
            // continue;
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                recreateSwapChain(pApp);
                continue;
            }
        }
        app_renderFrame(pApp);
    }
    vkDeviceWaitIdle(pApp->device);
}

void app_cleanup(VkApp *pApp) {
#ifdef ENABLE_VALIDATION_LAYERS
    DestroyDebugUtilsMessengerEXT(pApp->instance, pApp->debugMessenger, NULL);
#endif
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(pApp->device, pApp->imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(pApp->device, pApp->renderFinishedSemaphores[i], NULL);
        vkDestroyFence(pApp->device, pApp->inFlightFences[i], NULL);
    }
    vkDestroyCommandPool(pApp->device, pApp->commandPool, NULL);
    vkDestroyPipeline(pApp->device, pApp->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(pApp->device, pApp->pipelineLayout, NULL);
    vkDestroyRenderPass(pApp->device, pApp->renderPass, NULL);
    cleanupSwapChain(pApp);
        
    vkDestroyImage(pApp->device, pApp->textureImage, NULL);
    vkFreeMemory(pApp->device, pApp->textureImageMemory, NULL);
    vkDestroyImageView(pApp->device, pApp->textureImageView, NULL);
    vkDestroySampler(pApp->device, pApp->textureSampler, NULL);
    destroyUniformBuffers(pApp);
    vkDestroyDescriptorPool(pApp->device, pApp->descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(pApp->device, pApp->descriptorSetLayout, NULL);
    // free(pApp->imageAvailableSemaphores);
    // free(pApp->renderFinishedSemaphores);
    // free(pApp->inFlightFences);
    // free(pApp->commandBuffers);
    vkDestroyBuffer(pApp->device, pApp->vertexBuffer, NULL);
    vkFreeMemory(pApp->device, pApp->vertexBufferMemory, NULL);
    vkDestroyBuffer(pApp->device, pApp->indexBuffer, NULL);
    vkFreeMemory(pApp->device, pApp->indexBufferMemory, NULL);
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
