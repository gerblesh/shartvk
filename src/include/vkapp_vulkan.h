#include <stdint.h>
#include <vulkan/vulkan.h>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_vulkan.h"


#define DEVICE_EXTENSION_COUNT 1

const char *deviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const char *modelPath = "data/viking_room.obj";
const char *texturePath = "data/texture.png";

uint32_t modelVertexCount = 0;
Vertex *modelVertices;

uint32_t modelIndexCount = 0;
uint32_t *modelIndices;

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
void updateUniformBuffer(uint32_t currentImage, VkApp *pApp);

VkCommandBuffer beginSingleTimeCommands(VkApp *pApp);
void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkApp *pApp);
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkApp *pApp);
VkFormat findSupportedFormat(VkFormat *availableFormats, uint32_t availableFormatCount, VkImageTiling tiling, VkFormatFeatureFlags features, VkApp *pApp);
VkFormat findDepthFormat(VkApp *pApp);
bool hasStencilComponent(VkFormat format);
void createDepthResources(VkApp *pApp);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(uint32_t formatCount, VkSurfaceFormatKHR *availableFormats) {
    for (uint32_t i = 0; i < formatCount; i++) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats[i];
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(uint32_t presentModeCount, VkPresentModeKHR *availablePresentModes) {
    for (uint32_t i = 0; i < presentModeCount; i++) {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(SDL_Window *window, VkSurfaceCapabilitiesKHR capabilities) {
    // when the WM sets the current extent width to the max value of uint32_t
    // we can set the current extent to the window size in pixels
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    int width;
    int height;
    SDL_GL_GetDrawableSize(window, &width, &height);

    VkExtent2D actualExtent = {
        (uint32_t)width,
        (uint32_t)height
    };

    actualExtent.width = u32Clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = u32Clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details = {0};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
    if (formatCount == 0) {
        return details;
    }
    details.formatCount = formatCount;
    VkSurfaceFormatKHR formats[formatCount];
    details.formats = formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);
    if (presentModeCount == 0) {
        return details;
    }
    details.presentModeCount = presentModeCount;
    VkPresentModeKHR presentModes[presentModeCount];
    details.presentModes = presentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes);

    return details;
}

void createSwapChain(VkApp *pApp) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(pApp->physicalDevice, pApp->surface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formatCount, swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModeCount, swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(pApp->window, swapChainSupport.capabilities);

    // reccommended to request at least one more image than minimum so we don't have to wait for the driver
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    // 0 is a "special value" in that it denotes that there is no max image count for the swapchain
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = pApp->surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    QueueFamilyIndices indices = findQueueFamilies(pApp->physicalDevice, pApp->surface);

    UInt32Set uniqueQueueFamilies;
    initUInt32Set(&uniqueQueueFamilies);
    uint32SetInsert(&uniqueQueueFamilies, indices.graphicsFamily);
    uint32SetInsert(&uniqueQueueFamilies, indices.presentFamily);
    
    if (uniqueQueueFamilies.size > 1) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = uniqueQueueFamilies.size;
        swapChainCreateInfo.pQueueFamilyIndices = uniqueQueueFamilies.data;
    }

    if (vkCreateSwapchainKHR(pApp->device, &swapChainCreateInfo, NULL, &pApp->swapChain) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create swap chain!\n");
        exit(1);
    }
    vkGetSwapchainImagesKHR(pApp->device, pApp->swapChain, &imageCount, NULL);
    pApp->swapChainImages = (VkImage*)malloc(sizeof(VkImage) * imageCount);
    if (pApp->swapChainImages == NULL) {
        fprintf(stderr, "ERROR: unable to allocate for swap chain images!\n");
        exit(1);
    }
    pApp->swapChainImageCount = imageCount;
    vkGetSwapchainImagesKHR(pApp->device, pApp->swapChain, &imageCount, pApp->swapChainImages);
    pApp->swapChainImageFormat = surfaceFormat.format;
    pApp->swapChainExtent = extent;
}

void createImageViews(VkApp *pApp) {
    pApp->swapChainImageViews = (VkImageView*)malloc((size_t)pApp->swapChainImageCount * sizeof(VkImageView));
    if (pApp->swapChainImageViews == NULL) {
        fprintf(stderr, "ERROR: unable to allocate for swap chain image views!\n");
        exit(1);
    }
    
    for (uint32_t i = 0; i < pApp->swapChainImageCount; i++) {
        pApp->swapChainImageViews[i] = createImageView(pApp->swapChainImages[i], pApp->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, pApp);
    }
}

bool checkExtensionSupport(uint32_t requiredExtensionCount, const char **requiredExtensions) {
    // get the available layers
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

    VkExtensionProperties availableExtensions[extensionCount];
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, availableExtensions);

    for (int i = 0; i < requiredExtensionCount; i++) {
        const char *extensionName = requiredExtensions[i];
        bool extensionFound = false;
        for (int j = 0; j < extensionCount; j++) {
            VkExtensionProperties extensionProperties = availableExtensions[j];
            if (strcmp(extensionName, extensionProperties.extensionName) == 0) {
                extensionFound = true;
                printf("Found required instance extension: %s\n", extensionName);
                break;
            }
        }
        if (!extensionFound) {
            fprintf(stderr,"ERROR: failed to find required vulkan instance extension: %s!\n", extensionName);
            return false;
        }
    }

    return true;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // get the available layers
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties availableExtensions[extensionCount];
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

    for (uint32_t i = 0; i < DEVICE_EXTENSION_COUNT; i++) {
        const char *extensionName = deviceExtensions[i];
        bool extensionFound = false;
        for (int j = 0; j < extensionCount; j++) {
            VkExtensionProperties extensionProperties = availableExtensions[j];
            if (strcmp(extensionName, extensionProperties.extensionName) == 0) {
                extensionFound = true;
                printf("Found required device extension: %s\n", extensionName);
                break;
            }
        }
        if (!extensionFound) {
            fprintf(stderr,"ERROR: failed to find required vulkan device extension: %s!\n", extensionName);
            return false;
        }
    }

    return true;
}

void app_createVulkanInstance(VkApp *pApp) {
    if (!checkValidationLayerSupport()) {
        fprintf(stderr,"ERROR: validation layers requested, but not available!\n");
        exit(1);
    }

    printf("Hi from fucknuts!\n");
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

#ifdef ENABLE_VALIDATION_LAYERS
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
        createInfo.ppEnabledLayerNames = validationLayers;
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
#else
        createInfo.ppEnabledLayerNames = NULL;
        createInfo.enabledLayerCount = 0;
#endif

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
#ifdef ENABLE_VALIDATION_LAYERS
        extensions[extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        extensionCount++;
#endif

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    if (!checkExtensionSupport(sdlExtensionCount, sdlExtensions)) {
        fprintf(stderr,"ERROR: required vulkan extensions not supported!\n");
        exit(1);
    }

    VkResult result = vkCreateInstance(&createInfo, NULL, &pApp->instance);
    if (result != VK_SUCCESS) {
        printf("ERROR: Failed to create VkInstance");
        exit(1);
    }
    printf("Successfully created vkInstance!\n");
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
    if (!queueFamilies.requiredFamilesFound) {
        fprintf(stderr, "ERROR: device QueueFamilies do not support required buffers!");
        return false;
    }

    if (!checkDeviceExtensionSupport(device)) {
        fprintf(stderr, "ERROR: Required device extensions not supported!");
        return false;
    }

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
    printf("Supported image formats: %d\n", swapChainSupport.formatCount);
    printf("Supported present modes: %d\n", swapChainSupport.presentModeCount);
    if ((swapChainSupport.formatCount == 0) || (swapChainSupport.presentModeCount == 0)){
        fprintf(stderr, "ERROR: SwapChain is not adequately supported ");
        return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    if (!supportedFeatures.samplerAnisotropy) {
        return false;
    }

    return true;
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
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo logicalDeviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pQueueCreateInfos = queueCreateInfos,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &deviceFeatures,
        .enabledExtensionCount = DEVICE_EXTENSION_COUNT,
        .ppEnabledExtensionNames = deviceExtensions,
    };
#ifdef ENABLE_VALIDATION_LAYERS
        logicalDeviceCreateInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
        logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayers;
#else
        logicalDeviceCreateInfo.enabledLayerCount = 0;
        logicalDeviceCreateInfo.ppEnabledLayerNames = NULL;
#endif
    if (vkCreateDevice(pApp->physicalDevice, &logicalDeviceCreateInfo, NULL, &pApp->device) != VK_SUCCESS) {
        fprintf(stderr,"failed to create logical device!\n");
        exit(1);
    }
    vkGetDeviceQueue(pApp->device, indices.graphicsFamily, 0, &pApp->graphicsQueue);
    vkGetDeviceQueue(pApp->device, indices.presentFamily, 0, &pApp->presentQueue);
}

void createSurface(VkApp *pApp) {
    if (SDL_Vulkan_CreateSurface(pApp->window, pApp->instance, &pApp->surface) != SDL_TRUE) {
        fprintf(stderr, "failed to create window surface!\n");
        exit(1);
    }
}

VkShaderModule createShaderModule(VkApp *pApp, ShaderFile *file) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = file->size,
        .pCode = (uint32_t*)file->byteCode
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(pApp->device, &shaderModuleCreateInfo, NULL, &shaderModule) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: unable to create vulkan shader module!\n");
        exit(1);
    }
    return shaderModule;
}

void createGraphicsPipeline(VkApp *pApp) {
    ShaderFile vertexShader = {0};
    ShaderFile fragmentShader = {0};
    loadShaderFile("shaders/vert.spv", &vertexShader);
    loadShaderFile("shaders/frag.spv", &fragmentShader);

    VkShaderModule vertexShaderModule = createShaderModule(pApp, &vertexShader);
    VkShaderModule fragmentShaderModule = createShaderModule(pApp, &fragmentShader);

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertexShaderModule,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragmentShaderModule,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    VkVertexInputAttributeDescription attributeDescriptions[3];
    getVertexAttributeDescriptions(attributeDescriptions);
    VkVertexInputBindingDescription bindingDescription = getVertexBindingDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = 3,
        .pVertexAttributeDescriptions = attributeDescriptions
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) pApp->swapChainExtent.width,
        .height = (float) pApp->swapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = pApp->swapChainExtent
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = (VkStencilOpState){};
    depthStencil.back = (VkStencilOpState){};

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants[0] = 0.0f,
        .blendConstants[1] = 0.0f,
        .blendConstants[2] = 0.0f,
        .blendConstants[3] = 0.0f 
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &pApp->descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    if (vkCreatePipelineLayout(pApp->device, &pipelineLayoutInfo, NULL, &pApp->pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: pipeline layout creation failed!");
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = NULL,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pApp->pipelineLayout,
        .renderPass = pApp->renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    if (vkCreateGraphicsPipelines(pApp->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pApp->graphicsPipeline) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to create graphics pipeline!\n");
        exit(1);
    }

    vkDestroyShaderModule(pApp->device, fragmentShaderModule, NULL);
    vkDestroyShaderModule(pApp->device, vertexShaderModule, NULL);
    // free memory from loadShaderFile (no longer needed, we have the shader modules)
    free(vertexShader.byteCode);
    free(fragmentShader.byteCode);
}

void createRenderPass(VkApp *pApp) {
    VkAttachmentDescription depthAttachment = {0};
    depthAttachment.format = findDepthFormat(pApp);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {0};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment = {
        .format = pApp->swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = NULL,
        .pDepthStencilAttachment = &depthAttachmentRef,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    if (vkCreateRenderPass(pApp->device, &renderPassInfo, NULL, &pApp->renderPass) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create render pass!\n");
        exit(1);
    }
}

void createFramebuffers(VkApp *pApp) {
    pApp->swapChainFramebuffers = (VkFramebuffer*)malloc(pApp->swapChainImageCount * sizeof(VkFramebuffer));
    if (pApp->swapChainFramebuffers == NULL) {
        fprintf(stderr, "ERROR: unable to allocate for framebuffers\n");
        exit(1);
    }

    for (uint32_t i = 0; i < pApp->swapChainImageCount; i++) {
        VkImageView attachments[] = {pApp->swapChainImageViews[i], pApp->depthImageView};
        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = pApp->renderPass,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .width = pApp->swapChainExtent.width,
            .height = pApp->swapChainExtent.height,
            .layers = 1
        };

        if (vkCreateFramebuffer(pApp->device, &framebufferInfo, NULL, &pApp->swapChainFramebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "ERROR: vulkan unable to create framebuffer!\n");
            exit(1);
        }

    }
}

void createCommandPool(VkApp *pApp) {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(pApp->physicalDevice, pApp->surface);

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphicsFamily
    };

    if (vkCreateCommandPool(pApp->device, &poolInfo, NULL, &pApp->commandPool) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: unable to create command pool!\n");
        exit(1);
    }

}

void createCommandBuffers(VkApp *pApp) {
    // pApp->commandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = pApp->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    if (vkAllocateCommandBuffers(pApp->device, &allocInfo, pApp->commandBuffers) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: unable to allocate for command buffers!\n");
        exit(1);
    }
}

void recordCommandBuffer(VkApp *pApp, VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL
    };
    
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: unable to begin recording frambuffers!\n");
        exit(1);
    }

    
    VkClearValue clearColors[2];
    clearColors[0].color = (VkClearColorValue){{0.0f, 0.0f, 0.0f, 1.0f}};
    clearColors[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = pApp->renderPass,
        .framebuffer = pApp->swapChainFramebuffers[imageIndex],
        .renderArea.offset.x = 0,
        .renderArea.offset.y = 0,
        .renderArea.extent = pApp->swapChainExtent,
        .clearValueCount = 2,
        .pClearValues = clearColors
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pApp->graphicsPipeline);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)pApp->swapChainExtent.width,
        .height = (float)pApp->swapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = pApp->swapChainExtent
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


    VkBuffer vertexBuffers[] = {pApp->vertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, pApp->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pApp->pipelineLayout, 0, 1, &pApp->descriptorSets[pApp->currentFrame], 0, NULL);
    vkCmdDrawIndexed(commandBuffer, modelIndexCount, 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        fprintf(stderr,"ERROR: failed to record command buffer!");
        exit(1);
    }
}

void createSyncObjects(VkApp *pApp) {
    // pApp->imageAvailableSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    // pApp->renderFinishedSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    // pApp->inFlightFences = (VkFence*)malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(pApp->device, &semaphoreInfo, NULL, &pApp->imageAvailableSemaphores[i]) != VK_SUCCESS) {
            fprintf(stderr, "ERROR: Failed to create imageAvailableSemaphore!\n");
            exit(1);
        }
        if (vkCreateSemaphore(pApp->device, &semaphoreInfo, NULL, &pApp->renderFinishedSemaphores[i]) != VK_SUCCESS) {
            fprintf(stderr, "ERROR: Failed to create renderFinishedSemaphore!\n");
            exit(1);
        }
        if (vkCreateFence(pApp->device, &fenceInfo, NULL, &pApp->inFlightFences[i]) != VK_SUCCESS) {
            fprintf(stderr, "ERROR: Failed to create fence!\n");
            exit(1);
        }
    }
}

void cleanupSwapChain(VkApp *pApp) {
    for (uint32_t i = 0; i < pApp->swapChainImageCount; i++) {
        vkDestroyImageView(pApp->device, pApp->swapChainImageViews[i], NULL);
        vkDestroyFramebuffer(pApp->device, pApp->swapChainFramebuffers[i], NULL);
    }
    vkDestroySwapchainKHR(pApp->device, pApp->swapChain, NULL);
    vkDestroyImageView(pApp->device, pApp->depthImageView, NULL);
    vkDestroyImage(pApp->device, pApp->depthImage, NULL);
    vkFreeMemory(pApp->device, pApp->depthImageMemory, NULL);
    free(pApp->swapChainFramebuffers);
    free(pApp->swapChainImages);
    free(pApp->swapChainImageViews);
}

void recreateSwapChain(VkApp *pApp) {
    vkDeviceWaitIdle(pApp->device);

    cleanupSwapChain(pApp);

    createSwapChain(pApp);
    createImageViews(pApp);
    createDepthResources(pApp);
    createFramebuffers(pApp);
}

void app_renderFrame(VkApp *pApp) {
    vkWaitForFences(pApp->device, 1, &pApp->inFlightFences[pApp->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(pApp->device, pApp->swapChain, UINT64_MAX, pApp->imageAvailableSemaphores[pApp->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain(pApp);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr,"ERROR: failed to acquire swap chain image!");
        exit(1);
    }
    updateUniformBuffer(pApp->currentFrame, pApp);
    // Only reset the fence if we are submitting work
    vkResetFences(pApp->device, 1, &pApp->inFlightFences[pApp->currentFrame]);


    vkResetCommandBuffer(pApp->commandBuffers[pApp->currentFrame], 0);

    recordCommandBuffer(pApp, pApp->commandBuffers[pApp->currentFrame], imageIndex);


    VkSemaphore waitSemaphores[] = {pApp->imageAvailableSemaphores[pApp->currentFrame]};
    VkSemaphore signalSemaphores[] = {pApp->renderFinishedSemaphores[pApp->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &pApp->commandBuffers[pApp->currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    if (vkQueueSubmit(pApp->graphicsQueue, 1, &submitInfo, pApp->inFlightFences[pApp->currentFrame]) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to submit draw command buffer!");
        exit(1);
    }

    VkSwapchainKHR swapChains[] = {pApp->swapChain};
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
        .pResults = NULL
    };

    vkQueuePresentKHR(pApp->presentQueue, &presentInfo);

    pApp->currentFrame = (pApp->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    fprintf(stderr, "ERROR: failed to find suitable memory type!");
    exit(1);
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory, VkApp *pApp) {
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(pApp->device, &bufferInfo, NULL, pBuffer) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create vertex buffer!");
        exit(1);
    }
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(pApp->device, *pBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, pApp->physicalDevice);
    
    if (vkAllocateMemory(pApp->device, &allocInfo, NULL, pBufferMemory) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to allocate vertex buffer memory!");
        exit(1);
    }
    vkBindBufferMemory(pApp->device, *pBuffer, *pBufferMemory, 0);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkApp *pApp) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(pApp);

    VkBufferCopy copyRegion = {0};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, pApp);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkApp *pApp) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(pApp);
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = (VkOffset3D){0, 0, 0};
    region.imageExtent = (VkExtent3D){
        width,
        height,
        1
    };
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    endSingleTimeCommands(commandBuffer, pApp);
}

void createVertexBuffer(VkApp *pApp) {
    VkDeviceSize bufferSize = sizeof(Vertex) * modelVertexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory, pApp);

    void* data;
    vkMapMemory(pApp->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, modelVertices, (size_t)bufferSize);
    vkUnmapMemory(pApp->device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &pApp->vertexBuffer, &pApp->vertexBufferMemory, pApp);

    copyBuffer(stagingBuffer, pApp->vertexBuffer, bufferSize, pApp);

    vkDestroyBuffer(pApp->device, stagingBuffer, NULL);
    vkFreeMemory(pApp->device, stagingBufferMemory, NULL);

}
void createIndexBuffer(VkApp *pApp) {
    VkDeviceSize bufferSize = sizeof(uint32_t) * modelIndexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory, pApp);

    void* data;
    vkMapMemory(pApp->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, modelIndices, (size_t)bufferSize);
    vkUnmapMemory(pApp->device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &pApp->indexBuffer, &pApp->indexBufferMemory, pApp);

    copyBuffer(stagingBuffer, pApp->indexBuffer, bufferSize, pApp);

    vkDestroyBuffer(pApp->device, stagingBuffer, NULL);
    vkFreeMemory(pApp->device, stagingBufferMemory, NULL);

}


void createDescriptorSetLayout(VkApp *pApp) {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = NULL;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(pApp->device, &layoutInfo, NULL, &pApp->descriptorSetLayout) != VK_SUCCESS) {
        fprintf(stderr, "failed to create descriptor set layout!");
        exit(1);
    }
}

void updateUniformBuffer(uint32_t currentImage, VkApp *pApp) {
    double time = (double)SDL_GetTicks() / 1000.0;

    UniformBufferObject ubo;
    glm_mat4_identity(ubo.model);
    glm_rotate(ubo.model, time * glm_rad(90.0f), (vec3){0.0f, 0.0f, 1.0f});
    
    glm_mat4_identity(ubo.view);
    glm_lookat((vec3){2.0f, 2.0f, 2.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 0.0f, 1.0f}, ubo.view);

    glm_mat4_identity(ubo.proj);
    glm_perspective(glm_rad(45.0f), pApp->swapChainExtent.width / (float)pApp->swapChainExtent.height, 0.1f, 10.0f, ubo.proj);

    // Flip Y Axis because openGL is cring
    ubo.proj[1][1] *= -1;
    memcpy(pApp->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void createUniformBuffers(VkApp *pApp) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // pApp->uniformBuffers = (VkBuffer*)malloc(sizeof(VkBuffer) * MAX_FRAMES_IN_FLIGHT);
    // pApp->uniformBuffersMemory = (VkDeviceMemory*)malloc(sizeof(VkDeviceMemory) * MAX_FRAMES_IN_FLIGHT);
    // pApp->uniformBuffersMapped = (void**)malloc(sizeof(void*) * MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &pApp->uniformBuffers[i], &pApp->uniformBuffersMemory[i], pApp);

        vkMapMemory(pApp->device, pApp->uniformBuffersMemory[i], 0, bufferSize, 0, &pApp->uniformBuffersMapped[i]);
    }
}

void destroyUniformBuffers(VkApp *pApp) {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(pApp->device, pApp->uniformBuffers[i], NULL);
        vkFreeMemory(pApp->device, pApp->uniformBuffersMemory[i], NULL);
    }
    // free(pApp->uniformBuffers);
    // free(pApp->uniformBuffersMemory);
    // free(pApp->uniformBuffersMapped);
}

void createDescriptorPool(VkApp *pApp) {
    // VkDescriptorPoolSize poolSize = {0};
    // poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolSize poolSizes[2];
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    printf("create descriptor pool!\n");
    if (vkCreateDescriptorPool(pApp->device, &poolInfo, NULL, &pApp->descriptorPool) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create descriptor pool!");
        exit(1);
    }
    printf("create descriptor pool!\n");
}

void createDescriptorSets(VkApp *pApp) {
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
    // Initialize the array with the same descriptorSetLayout value
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        layouts[i] = pApp->descriptorSetLayout;
    }
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pApp->descriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(pApp->device, &allocInfo, pApp->descriptorSets) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to allocate descriptor sets!");
        exit(1);
    }
    printf("after create descriptor sets!\n");
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo = {0};
        bufferInfo.buffer = pApp->uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo = {0};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = pApp->textureImageView;
        imageInfo.sampler = pApp->textureSampler;

        VkWriteDescriptorSet descriptorWrites[2];

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].pNext = NULL;
        descriptorWrites[0].dstSet = pApp->descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = NULL;
        descriptorWrites[0].pTexelBufferView = NULL;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].pNext = NULL;
        descriptorWrites[1].dstSet = pApp->descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(pApp->device, 2, descriptorWrites, 0, NULL);
    }
    printf("after create descriptor sets!\n");
}

void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *pImage, VkDeviceMemory *pImageMemory, VkApp *pApp) {
    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    if (vkCreateImage(pApp->device, &imageInfo, NULL, pImage) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create VkImage!");
        exit(1);
    }
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(pApp->device, *pImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, pApp->physicalDevice);

    if (vkAllocateMemory(pApp->device, &allocInfo, NULL, pImageMemory) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to allocate image memory!");
        exit(1);
    }

    vkBindImageMemory(pApp->device, *pImage, *pImageMemory, 0);
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkApp *pApp) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(pApp);

    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        fprintf(stderr, "ERROR: unsupportted layout transition!");
        exit(1);
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer, pApp);
}

void createTextureImageView(VkApp *pApp) {
    pApp->textureImageView = createImageView(pApp->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, pApp);
}

void createTextureSampler(VkApp *pApp) {
    VkSamplerCreateInfo samplerInfo = {0};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(pApp->physicalDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(pApp->device, &samplerInfo, NULL, &pApp->textureSampler) != VK_SUCCESS) {
        fprintf(stderr, "failed to create texture sampler!");
        exit(1);
    }
}

void createTextureImage(VkApp *pApp) {
    // Load image with SDL_image
    SDL_Surface *originalSurface = IMG_Load(texturePath);

    if (originalSurface == NULL) {
        fprintf(stderr, "ERROR: image can't load: %s\n", SDL_GetError());
        exit(1);
    }
    // convert to RGBA because SDL is cringe and by default loads in RGB??
    // idk this is wack

    SDL_Surface* surfaceRGBA = SDL_ConvertSurfaceFormat(originalSurface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(originalSurface);


    if (surfaceRGBA == NULL) {
        fprintf(stderr, "Failed to convert surface to RGBA format: %s\n", SDL_GetError());
        exit(1);
    }


    printf("bytes per pixel: %d\n", surfaceRGBA->format->BytesPerPixel);
    // hacky way to work around image formats, since SDL uses RGB and Vulkan uses RGBA (probably will cause problems down the line but idc)
    size_t imageSize = surfaceRGBA->w * surfaceRGBA->h * surfaceRGBA->format->BytesPerPixel;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory, pApp);

    void* data;
    vkMapMemory(pApp->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, surfaceRGBA->pixels, imageSize);
    vkUnmapMemory(pApp->device, stagingBufferMemory);

    createImage(surfaceRGBA->w, surfaceRGBA->h, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &pApp->textureImage, &pApp->textureImageMemory, pApp);
    transitionImageLayout(pApp->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pApp);
    copyBufferToImage(stagingBuffer, pApp->textureImage, surfaceRGBA->w, surfaceRGBA->h, pApp);
    transitionImageLayout(pApp->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pApp);

    vkDestroyBuffer(pApp->device, stagingBuffer, NULL);
    vkFreeMemory(pApp->device, stagingBufferMemory, NULL);

    SDL_FreeSurface(surfaceRGBA);
}

VkCommandBuffer beginSingleTimeCommands(VkApp *pApp) {
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pApp->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(pApp->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);


    return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkApp *pApp) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(pApp->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(pApp->graphicsQueue);

    vkFreeCommandBuffers(pApp->device, pApp->commandPool, 1, &commandBuffer);
}

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkApp *pApp) {
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(pApp->device, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
        fprintf(stderr, "failed to create texture image view!");
        exit(1);
    }

    return imageView;
}

VkFormat findSupportedFormat(VkFormat *availableFormats, uint32_t availableFormatCount, VkImageTiling tiling, VkFormatFeatureFlags features, VkApp *pApp) {
    for (uint32_t i = 0; i < availableFormatCount; i++) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(pApp->physicalDevice, availableFormats[i], &properties);
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            return availableFormats[i];
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            return availableFormats[i];
        }
    }
    

    fprintf(stderr, "ERORR: Very cringe no format");
    exit(1);
}

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat findDepthFormat(VkApp *pApp) {
    VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return findSupportedFormat(
        formats,
        3,
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        pApp
    );
}

void createDepthResources(VkApp *pApp) {
    VkFormat depthFormat = findDepthFormat(pApp);
    createImage(pApp->swapChainExtent.width, pApp->swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &pApp->depthImage, &pApp->depthImageMemory, pApp);
    pApp->depthImageView = createImageView(pApp->depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, pApp);
    transitionImageLayout(pApp->depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, pApp);
}


void loadFile(void *ctx, const char * filename, const int is_mtl, const char *obj_filename, char ** buffer, size_t * len)
{
    long string_size = 0, read_size = 0;
    FILE * handler = fopen(filename, "r");

    if (handler) {
        fseek(handler, 0, SEEK_END);
        string_size = ftell(handler);
        rewind(handler);
        *buffer = (char *) malloc(sizeof(char) * (string_size + 1));
        read_size = fread(*buffer, sizeof(char), (size_t) string_size, handler);
        (*buffer)[string_size] = '\0';
        if (string_size != read_size) {
            free(buffer);
            buffer = NULL;
        }
        fclose(handler);
    }

    *len = read_size;
}


void loadModel() {
    printf("INFO: Loading model: %s!\n", modelPath);
    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    tinyobj_attrib_t attrib;
    tinyobj_shape_t *shapes;
    tinyobj_material_t *materials;
    
    size_t num_shapes = 0;
    size_t num_materials = 0;

    int ret =
        tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
                          &num_materials, modelPath, loadFile, NULL, flags);
    if (ret != TINYOBJ_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load obj file!");
        exit(1);
    }
    modelVertices = (Vertex*)malloc((size_t)attrib.num_vertices * sizeof(Vertex));
    modelIndices = (uint32_t*)malloc((size_t)attrib.num_faces * sizeof(uint32_t));

    modelIndexCount = attrib.num_faces;
    modelVertexCount = attrib.num_vertices;
    printf("Num Faces: %u, Num Verts: %u\n", attrib.num_faces, attrib.num_vertices);
    for (uint32_t i; i < attrib.num_faces; i++) {
        Vertex vertex = {};
        vertex.pos[0] = attrib.vertices[3 * attrib.faces[i].v_idx + 0];
        vertex.pos[1] = attrib.vertices[3 *attrib.faces[i].v_idx + 1];
        vertex.pos[2] = attrib.vertices[3 *attrib.faces[i].v_idx + 2];
        
        
        vertex.texCoord[0] = attrib.texcoords[2 * attrib.faces[i].vt_idx + 0];
        vertex.texCoord[1] = 1.0f - attrib.texcoords[2 * attrib.faces[i].vt_idx + 1];
        
        vertex.color[0] = 1.0f;
        vertex.color[1] = 1.0f;
        vertex.color[2] = 1.0f;

        modelIndices[i] = attrib.faces[i].v_idx;
        modelVertices[attrib.faces[i].v_idx] = vertex;
    }
    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
}
