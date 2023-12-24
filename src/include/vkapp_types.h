typedef struct {
    uint32_t width;
    uint32_t height;
    char *title;
    SDL_Window *window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
} VkApp;

void populateVkApp(uint32_t width,uint32_t height, char *title, VkApp *pApp) {
    pApp->title = title;
    pApp->width = width;
    pApp->width = height;
    pApp->window = NULL;
    pApp->instance = VK_NULL_HANDLE;
    pApp->debugMessenger = VK_NULL_HANDLE;
    pApp->surface = VK_NULL_HANDLE;
    pApp->physicalDevice = VK_NULL_HANDLE;
    pApp->device = VK_NULL_HANDLE;
    pApp->graphicsQueue = VK_NULL_HANDLE;
}
