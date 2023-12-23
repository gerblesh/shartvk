#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    char* title;
    GLFWwindow* window;
    VkInstance instance;
} VkApp;

// Vulkan shtuff
// TODO: reorg later :D

void app_createVulkanInstance(VkApp* app) {
    // initialize the extensions array
    // app->extensionCount = 1;
    // app->extensions = (VkExtensionProperties*)malloc(app->extensionCount * sizeof(VkExtensionProperties));

    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = app->title;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = NULL;

    // get the amount of instance extensions needed to allocate
    uint32_t instnaceExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &instnaceExtensionCount, NULL);
    VkExtensionProperties* instanceExtensions = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &instnaceExtensionCount, instanceExtensions);
    // get the glfw extensions:
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // get list of all extensions needed:
    extensions
    for (int i = 0; i<extensionCount + glfwExtensionCount) {
        
    }

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;


    // allocate for the function (for some reason thems don't allocate for ya)
    // app->instance = (VkInstance*)malloc(sizeof(VkInstance));
    printf("heyo, before the vkCreateInstance!\n");
    VkResult result = vkCreateInstance(&createInfo, NULL, &app->instance);
    printf("heyo, after the vkCreateInstance!\n");
    free(extensions);
    assert(result != VK_SUCCESS);
}

// GLFW shtuff

void app_initVulkan(VkApp* app) {
    app_createVulkanInstance(app);
}

void app_initWindow(VkApp* app) {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    app->window = glfwCreateWindow(app->width, app->height, app->title, NULL, NULL);
}

void app_mainLoop(VkApp* app) {
    while (!glfwWindowShouldClose(app->window)) {
        glfwPollEvents();
    }
}

void app_cleanup(VkApp* app) {
    // delete vk instance I guess
    vkDestroyInstance(app->instance, NULL);
    // free array of extensions (malloced in app_createVulkanInstance)
    
    glfwDestroyWindow(app->window);
    glfwTerminate();
}

int app_run(VkApp* app) {
    app_initVulkan(app);
    app_mainLoop(app);
    app_cleanup(app);
    return 0;
}


