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
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    uint32_t swapChainImageCount;
    VkImage *swapChainImages;
    VkFormat swapChainImageFormat;
    VkImageView *swapChainImageViews;
    VkExtent2D swapChainExtent;
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
    pApp->presentQueue = VK_NULL_HANDLE;
    pApp->swapChain = VK_NULL_HANDLE;
    pApp->swapChainImages = NULL;
    pApp->swapChainImageViews = NULL;
}

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatCount;
    VkSurfaceFormatKHR *formats;
    uint32_t presentModeCount;
    VkPresentModeKHR *presentModes;
} SwapChainSupportDetails;

typedef struct {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool requiredFamilesFound;
} QueueFamilyIndices;

#define VKAPP_MAX_SET_SIZE 10  // Adjust the size according to your needs

typedef struct {
    uint32_t data[VKAPP_MAX_SET_SIZE];
    uint32_t size;
} UInt32Set;

void initUInt32Set(UInt32Set *set) {
    set->size = 0;
}

bool uint32SetContains(const UInt32Set *set, uint32_t value) {
    for (size_t i = 0; i < set->size; ++i) {
        if (set->data[i] == value) {
            return true;
        }
    }
    return false;
}

void uint32SetInsert(UInt32Set *set, uint32_t value) {
    if (!uint32SetContains(set, value) && set->size < VKAPP_MAX_SET_SIZE) {
        set->data[set->size++] = value;
    }
}

uint32_t u32Clamp(uint32_t n, uint32_t min, uint32_t max) {
    if (n > max) return max;
    if (n < min) return min;
    return n;
}
