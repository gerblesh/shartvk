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

void app_createVulkanInstance(VkApp* pApp) {
    // initialize the extensions array
    // app->extensionCount = 1;
    // app->extensions = (VkExtensionProperties*)malloc(app->extensionCount * sizeof(VkExtensionProperties));

    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = pApp->title;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        createInfo.flags = 0,
        createInfo.pNext = NULL,
        createInfo.pApplicationInfo = &appInfo,
        createInfo.enabledLayerCount = 0,
        createInfo.ppEnabledLayerNames = NULL
    };

    // get the amount of instance extensions needed to allocate
    // uint32_t instnaceExtensionCount = 0;
    // vkEnumerateInstanceExtensionProperties(NULL, &instnaceExtensionCount, NULL);
    // get the glfw extensions:
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    printf("glfw extensions: %d \n", glfwExtensionCount);
    // get list of all extensions needed:

    

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    // get supported extensions
    // VkExtensionProperties* instanceExtensions = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
    // vkEnumerateInstanceExtensionProperties(NULL, &instnaceExtensionCount, instanceExtensions);


    // allocate for the function (for some reason thems don't allocate for ya)
    // app->instance = (VkInstance*)malloc(sizeof(VkInstance));
    printf("heyo, before the vkCreateInstance!\n");
    VkResult result = vkCreateInstance(&createInfo, NULL, &pApp->instance);
    printf("heyo, after the vkCreateInstance!\n");
    // free(extensions);
    assert(result == VK_SUCCESS);
}

// GLFW shtuff

void app_initVulkan(VkApp* pApp) {
    app_createVulkanInstance(pApp);
}

void app_initWindow(VkApp* pApp) {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    pApp->window = glfwCreateWindow(pApp->width, pApp->height, pApp->title, NULL, NULL);
}

void app_mainLoop(VkApp* pApp) {
    while (!glfwWindowShouldClose(pApp->window)) {
        glfwPollEvents();
    }
}

void app_cleanup(VkApp* pApp) {
    vkDestroyInstance(pApp->instance, NULL);
    glfwDestroyWindow(pApp->window);
    glfwTerminate();
}

int app_run(VkApp* pApp) {
    app_initVulkan(pApp);
    app_mainLoop(pApp);
    app_cleanup(pApp);
    return 0;
}
