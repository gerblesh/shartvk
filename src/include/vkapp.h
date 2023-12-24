#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <vulkan/vulkan.h>
#include "SDL.h"

#define ENABLE_VALIDATION_LAYERS true
#define VALIDATION_LAYER_COUNT 1

typedef struct {
    uint32_t width;
    uint32_t height;
    char* title;
    SDL_Window* window;
    VkInstance instance;
} VkApp;


// Vulkan stuff
const char *validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

char** getRequiredExtensions() {
    
}

bool checkValidationLayerSupport() {
    // if the validation layers are disabled, immediately exit out
    if (! (ENABLE_VALIDATION_LAYERS)) {
        return true;
    }
    // get the available layers
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    // check if the requested validation layers are available
    for (int i = 0; i < VALIDATION_LAYER_COUNT; i++) {
        const char* layerName = validationLayers[i];
        bool layerFound = false;

        for (int j = 0; j < layerCount; j++) {
            VkLayerProperties layerProperties = availableLayers[j];
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
    
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

bool checkExtensionSupport(uint32_t requiredExtensionCount, const char **requiredExtensions) {
    // get the available layers
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

    VkExtensionProperties availableExtensions[extensionCount];
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, availableExtensions);

    for (int i = 0; i < requiredExtensionCount; i++) {
        char *extensionName = requiredExtensions[i];
        bool extensionFound = false;
        for (int j = 0; j < extensionCount; j++) {
            VkExtensionProperties extensionProperties = availableExtensions[j];
            if (strcmp(extensionName, extensionProperties.extensionName) == 0) {
                extensionFound = true;
                break;
            }
        }
        if (!extensionFound) {
            fprintf(stderr,"ERROR: failed to find required vulkan extension: %s!\n", extensionName);
            return false;
        }
    }

    return true;
}

void app_createVulkanInstance(VkApp* pApp) {
    if (!checkValidationLayerSupport()) {
        fprintf(stderr,"ERROR: validation layers requested, but not available!\n");
        exit(1);
    }
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
    };
    
    
    if (ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
        createInfo.ppEnabledLayerNames = validationLayers;
    } else {
        createInfo.ppEnabledLayerNames = NULL;
        createInfo.enabledLayerCount = 0;
    }

    // get extension count first
    uint32_t sdlExtensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(pApp->window, &sdlExtensionCount, NULL)) {
        fprintf(stderr, "ERROR: Failed to get instance extensions count. %s\n", SDL_GetError());
        exit(1);
    }
    printf("SDL extension amount: %d\n", sdlExtensionCount);
    // then allocate and get the extension array
    const char* sdlExtensions[sdlExtensionCount];
    if (!SDL_Vulkan_GetInstanceExtensions(pApp->window, &sdlExtensionCount, sdlExtensions)) {
        fprintf(stderr, "ERROR: Failed to get instance extensions count. %s\n", SDL_GetError());
        exit(1);
    }

    uint32_t extensionCount = sdlExtensionCount;
    const char *extensions[sdlExtensionCount + 1];
    if (ENABLE_VALIDATION_LAYERS) {
        extensionCount++;
        for (int i = 0; i < sdlExtensionCount; i++) {
            extensions[i] = sdlExtensions[i];
        }
        extensions[sdlExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }


    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    if (!checkExtensionSupport(sdlExtensionCount, sdlExtensions)) {
        fprintf(stderr,"ERROR: required vulkan extensions not supported!\n");
        exit(1);
    }

    VkResult result = vkCreateInstance(&createInfo, NULL, &pApp->instance);
    printf("Successfully created vkInstance!\n");
    assert(result == VK_SUCCESS);
}

// end of vulkan stuff

void app_initVulkanSDL(VkApp* pApp) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        exit(1);
    }

    pApp->window = SDL_CreateWindow(pApp->title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, pApp->width, pApp->height, SDL_WINDOW_VULKAN);
    if (!pApp->window) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        exit(1);
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
