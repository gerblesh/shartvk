#include "cglm/cglm.h"
#define MAX_FRAMES_IN_FLIGHT 2

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
    VkFramebuffer *swapChainFramebuffers;
    VkExtent2D swapChainExtent;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT];
    void* uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    uint32_t currentFrame;
} VkApp;

void populateVkApp(uint32_t width,uint32_t height, char *title, VkApp *pApp) {
    pApp->title = title;
    pApp->width = width;
    pApp->height = height;
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
    pApp->pipelineLayout = VK_NULL_HANDLE;
    pApp->currentFrame = 0;
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

#define VKAPP_MAX_SET_SIZE 10

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

typedef struct {
    size_t size;
    char *byteCode;
} ShaderFile;

void loadShaderFile(const char *filePath, ShaderFile *shaderFile) {
    FILE *pFile = fopen(filePath, "rb");

    if (pFile == NULL) {
        fprintf(stderr, "ERROR: unable to open file: %s\n", filePath);
        exit(1);
    }

    fseek(pFile, 0L, SEEK_END);
    shaderFile->size = ftell(pFile);

    // Allocate memory for the code
    shaderFile->byteCode = (char *)malloc(shaderFile->size);
    if (shaderFile->byteCode == NULL) {
        fclose(pFile);
        fprintf(stderr, "ERROR: Failed to allocate memory for file: %s\n", filePath);
        exit(1);
    }

    fseek(pFile, 0L, SEEK_SET);
    fread(shaderFile->byteCode, shaderFile->size, sizeof(char), pFile);
    fclose(pFile);
}

typedef struct {
    vec2 pos;
    vec3 color;
    vec2 texCoord;
} Vertex;

typedef struct {
    mat4 model;
    mat4 view;
    mat4 proj;
} UniformBufferObject;

VkVertexInputBindingDescription getVertexBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {0};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

void getVertexAttributeDescriptions(VkVertexInputAttributeDescription *attributeDescriptions) {
    // setting attribute description for vertex position
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    // setting attribute description for vertex color
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
}
