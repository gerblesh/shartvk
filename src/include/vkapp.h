#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef struct {
    uint width;
    uint height;
    GLFWwindow* window;
} VKApp;

void app_initWindow(VKApp* app) {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    app->window = glfwCreateWindow(app->width, app->height, "Vulkan", NULL, NULL);
}

void app_initVulkan(VKApp* app) {
    return;
}

void app_mainLoop(VKApp* app) {
    while (!glfwWindowShouldClose(app->window)) {
        glfwPollEvents();
    }
}

void app_cleanup(VKApp* app) {
    glfwDestroyWindow(app->window);
    glfwTerminate();
}

int app_run(VKApp* app) {
    app_initVulkan(app);
    app_mainLoop(app);
    app_cleanup(app);
    return 0;
}


