/* vulkan.h */

#include "defines.h"

#define VKS VK_SUCCESS
#define VK_IMAGE_COUNT 3

typedef struct {
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    u32 DeviceQueueIndex;

    VkSwapchainKHR swapChain;
    VkImage swapChainImages[VK_IMAGE_COUNT];
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkImageView swapChainImageViews[VK_IMAGE_COUNT];
    VkRenderPass renderPass;
    VkFramebuffer swapChainFramebuffers[VK_IMAGE_COUNT];
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffers[VK_IMAGE_COUNT];

    VkSemaphore imageAvailableSemaphores[VK_IMAGE_COUNT];
    VkSemaphore renderFinishedSemaphores[VK_IMAGE_COUNT];
    VkFence inFlightFences[VK_IMAGE_COUNT];
} vk_context;

void initWindow(void);
void createInstance(void);
void createSurface(void);
void pickPhysicalDevice(void);
void createLogicalDevice(void);
void createSwapChain(void);
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 miplevels);
void createImageViews(void);
void createRenderPass(void);
void createFramebuffers(void);
VkShaderModule createShaderModule(const u32 *code, u32 size);
void createGraphicsPipeline(void);
void createCommandPool(void);
void createCommandBuffers(void);
void createSyncObjects(void);
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
void drawFrame(void);
void initVulkan(void);
void cleanup_swap_chain(void);
void cleanup(void);
void handle_window_resize(void);
