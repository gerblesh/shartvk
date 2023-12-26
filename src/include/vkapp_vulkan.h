#include <vulkan/vulkan.h>
#include "SDL.h"
#include "SDL_vulkan.h"

#define DEVICE_EXTENSION_COUNT 1

const char *deviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

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
    vkGetSwapchainImagesKHR(pApp->device, pApp->swapChain, &imageCount, pApp->swapChainImages);
    pApp->swapChainImageFormat = surfaceFormat.format;
    pApp->swapChainExtent = extent;
}

void createImageViews(VkApp *pApp) {
    pApp->swapChainImageViews = (VkImageView*)malloc(sizeof(VkImageView) * pApp->swapChainImageCount);
    if (pApp->swapChainImageViews == NULL) {
        fprintf(stderr, "ERROR: unable to allocate for swap chain image views!\n");
        exit(1);
    }
    
    for (uint32_t i = 0; i < pApp->swapChainImageCount; i++) {
        VkImageViewCreateInfo createInfo ={
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = pApp->swapChainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = pApp->swapChainImageFormat,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1
        };
        if (vkCreateImageView(pApp->device, &createInfo, NULL, &pApp->swapChainImageViews[i]) != VK_SUCCESS) {
            printf("Failed to create image views!\n");
            exit(1);
        }
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
    // free memory from loadShaderFile (no longer needed, we have the shader modules)
    free(vertexShader.byteCode);
    free(fragmentShader.byteCode);

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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
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
        .depthBiasSlopeFactor = 0.0f
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
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
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
        .pDepthStencilState = NULL,
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
}

void createRenderPass(VkApp *pApp) {
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
        .pDepthStencilAttachment = NULL,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
        .pDependencies = NULL
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
        VkImageView attachments[] = {
            pApp->swapChainImageViews[i]
        };
        
        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = pApp->renderPass,
            .attachmentCount = 1,
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

void createCommandBuffer(VkApp *pApp) {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = pApp->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(pApp->device, &allocInfo, &pApp->commandBuffer) != VK_SUCCESS) {
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

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = pApp->renderPass,
        .framebuffer = pApp->swapChainFramebuffers[imageIndex],
        .renderArea.offset = {0, 0},
        .renderArea.extent = pApp->swapChainExtent,
        .clearValueCount = 1,
        .pClearValues = &clearColor
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

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        fprintf(stderr,"ERROR: failed to record command buffer!");
        exit(1);
    }
}

void app_render(VkApp *pApp) {
    return;
}

