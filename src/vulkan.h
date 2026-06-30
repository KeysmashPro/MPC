/* vulkan.c */

#include "base.h"

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
