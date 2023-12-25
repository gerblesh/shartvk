#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"

typedef struct {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool requiredFamilesFound;
} QueueFamilyIndices;

void app_createVulkanInstance(VkApp *pApp) {
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
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
        createInfo.ppEnabledLayerNames = validationLayers;
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
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
    for (int i = 0; i < sdlExtensionCount; i++) {
            extensions[i] = sdlExtensions[i];
    }
    if (ENABLE_VALIDATION_LAYERS) {
        extensions[extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        extensionCount++;
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

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    QueueFamilyIndices indices = {0};
    bool graphicsFamilyFound = false;
    bool presentFamilyFound = false;
    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            printf("Found supported queueFamily for graphics! Index: %d\n", i);
            indices.graphicsFamily = i;
            graphicsFamilyFound = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            printf("Found supported queueFamily for presentation! Index: %d\n", i);
            indices.presentFamily = i;
            presentFamilyFound = true;
        }

        indices.requiredFamilesFound = graphicsFamilyFound && presentFamilyFound;
        if (indices.requiredFamilesFound) break;
    }
    return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices queueFamilies = findQueueFamilies(device, surface);
    return queueFamilies.requiredFamilesFound;
}

void pickPhysicalDevice(VkApp *pApp) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(pApp->instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        fprintf(stderr, "failed to find GPUs with Vulkan support!\n");
        exit(1);
    }

    VkPhysicalDevice physicalDevices[deviceCount];
    vkEnumeratePhysicalDevices(pApp->instance, &deviceCount, physicalDevices);

    for (int i = 0; i < deviceCount; i++) {
        VkPhysicalDevice device = physicalDevices[i];
        if (isDeviceSuitable(device, pApp->surface)) {
            pApp->physicalDevice = device;
            break;
        }
    }
    if (pApp->physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "failed to a suitable GPU!\n");
        exit(1);
    } else {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(pApp->physicalDevice, &deviceProperties);
        printf("Found a suitable GPU: %s!\n", deviceProperties.deviceName);
    }
}

void createLogicalDevice(VkApp *pApp) {
    QueueFamilyIndices indices = findQueueFamilies(pApp->physicalDevice, pApp->surface);
   
    UInt32Set uniqueQueueFamilies;
    initUInt32Set(&uniqueQueueFamilies);
    uint32SetInsert(&uniqueQueueFamilies, indices.graphicsFamily);
    uint32SetInsert(&uniqueQueueFamilies, indices.presentFamily);

    VkDeviceQueueCreateInfo queueCreateInfos[uniqueQueueFamilies.size];
    float queuePriority = 1.0f;
    for (int i = 0; i < uniqueQueueFamilies.size; i++) {
        VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = indices.graphicsFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
        queueCreateInfos[i] = queueCreateInfo;
    }

    // initialize all features to false by default
    VkPhysicalDeviceFeatures deviceFeatures = {VK_FALSE};

    VkDeviceCreateInfo logicalDeviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pQueueCreateInfos = queueCreateInfos,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &deviceFeatures,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };
    if (ENABLE_VALIDATION_LAYERS) {
        logicalDeviceCreateInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
        logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayers;
    } else {
        logicalDeviceCreateInfo.enabledLayerCount = 0;
        logicalDeviceCreateInfo.ppEnabledLayerNames = NULL;
    }
    if (vkCreateDevice(pApp->physicalDevice, &logicalDeviceCreateInfo, NULL, &pApp->device) != VK_SUCCESS) {
        fprintf(stderr,"failed to create logical device!\n");
        exit(1);
    }
    vkGetDeviceQueue(pApp->device, indices.graphicsFamily, 0, &pApp->graphicsQueue);
}

void createSurface(VkApp *pApp) {
    if (SDL_Vulkan_CreateSurface(pApp->window, pApp->instance, &pApp->surface) != SDL_TRUE) {
        fprintf(stderr, "failed to create window surface!\n");
        exit(1);
    }
}

void app_render(VkApp *pApp) {
    return;
}

