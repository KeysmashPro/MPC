#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "./shaders/shaderdump.h"
#include "./defines.h"

#define VKS VK_SUCCESS
#define W_WIDTH  720
#define W_HEIGHT 480
#define IMAGE_COUNT 3


/* VARIABLES */

VkFormat PREFERRED_COLOR_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
VkColorSpaceKHR PREFERRED_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

GLFWwindow *window;
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue queue;
u32 DeviceQueueIndex;


VkSwapchainKHR swapChain;
VkImage swapChainImages[IMAGE_COUNT];
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent; 
VkImageView swapChainImageViews[IMAGE_COUNT];
VkRenderPass renderPass;
VkFramebuffer swapChainFramebuffers[IMAGE_COUNT];
VkPipeline graphicsPipeline;
VkCommandPool commandPool;
VkCommandBuffer commandBuffers[IMAGE_COUNT];

VkSemaphore imageAvailableSemaphores[IMAGE_COUNT];
VkSemaphore renderFinishedSemaphores[IMAGE_COUNT];
VkFence inFlightFences[IMAGE_COUNT];
u32 currentFrame = 0;

void initWindow()
{
    if(!glfwInit()) { fail("Failed to glfwInit!"); }
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "UCP", NULL, NULL);
    if (!window) { glfwTerminate(); fail("Failed to create window!"); }
    glfwShowWindow(window);
}

void createInstance()
{
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ucp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "rainbow";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    u32 glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (!glfwExtensions) { fail("Failed to get GLFW required extensions!"); }

    const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
    u32 layerCount = 0;

#ifdef DEBUG
    layerCount = 1; 
    info("Validation layers requested");
#endif

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = validationLayers;

    VkResult instance_result = vkCreateInstance(&createInfo, NULL, &instance);
    if (instance_result != VKS) {
        fail("Vulkan instance creation failed! Error code: %d", instance_result);
    }
    info("Vulkan instance created successfully");
}

void createSurface()
{
    VkResult res = glfwCreateWindowSurface(instance, window, NULL, &surface);
    if (res != VKS) { fail("failed to create window surface!"); }
}

void pickPhysicalDevice()
{
    debug("Enter func pickPhysicalDevice");
    u32 deviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, NULL) != VKS) { fail("Fail physical devices"); }
    if (deviceCount == 0) { fail("Found 0 GPUs with Vulkan support!\n"); }

    VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    physicalDevice = devices[0];
    free(devices);
    
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    info("Selected GPU: %s", deviceProperties.deviceName);
}

void createLogicalDevice()
{
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
    
    u32 selectedIndex = UINT32_MAX;
    for (u32 i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        VkBool32 presentSupport = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport) {
          selectedIndex = i;
          info("Selected queue family %d (graphics + present)", i);
          break;
        }
      }
    }
    free(queueFamilies);
    
    if (selectedIndex == UINT32_MAX) { fail("No suitable queue family found!"); }
    
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = selectedIndex;
    queueCreateInfo.queueCount = 1;
    f32 queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    
    // Add feature structure for shaderDrawParameters
    VkPhysicalDeviceVulkan11Features vulkan11Features = {0};
    vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan11Features.shaderDrawParameters = VK_TRUE;
    
    const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
    u32 layerCount = 0;
    
    #ifdef DEBUG
    layerCount = 1;
    #endif
    
    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = layerCount > 0 ? validationLayers : NULL;
    
    // Add the feature structure
    createInfo.pNext = &vulkan11Features;
    
    VkResult result = vkCreateDevice(physicalDevice, &createInfo, NULL, &device);
    if (result != VKS) { fail("Failed to create logical device! Error: %d", result); }
    vkGetDeviceQueue(device, selectedIndex, 0, &queue);
    DeviceQueueIndex = selectedIndex;
}

void createSwapChain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult capResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
    if (capResult != VKS) { fail("Failed to get surface capabilities! Error: %d", capResult); }
    
    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
    if (!formatCount) { fail("No surface formats found!"); }
    VkSurfaceFormatKHR* formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);
    
    VkSurfaceFormatKHR selectedFormat = {PREFERRED_COLOR_FORMAT, PREFERRED_COLOR_SPACE};
    
    i32 formatFound = 0;
    for (u32 i = 0; i < formatCount; i++) {
        if (formats[i].format == PREFERRED_COLOR_FORMAT && formats[i].colorSpace == PREFERRED_COLOR_SPACE) {
            selectedFormat = formats[i];
            formatFound = 1;
            break;
        }
    }

    
    if (!formatFound) {
        selectedFormat = formats[0];
        info("Using fallback format: %d", selectedFormat.format);
    }
    free(formats);
    
    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
    if (presentModeCount == 0) {
        fail("No present modes found!");
    }
    
    VkPresentModeKHR* presentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);
    
    VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            selectedPresentMode = presentModes[i];
            break;
        }
    }
    free(presentModes);
    
    VkExtent2D extent = {W_WIDTH, W_HEIGHT};
    if (capabilities.currentExtent.width != 0xFFFFFFFF) {
        extent = capabilities.currentExtent;
    } else {
        if (extent.width < capabilities.minImageExtent.width)
            extent.width = capabilities.minImageExtent.width;
        if (extent.width > capabilities.maxImageExtent.width)
            extent.width = capabilities.maxImageExtent.width;
        if (extent.height < capabilities.minImageExtent.height)
            extent.height = capabilities.minImageExtent.height;
        if (extent.height > capabilities.maxImageExtent.height)
            extent.height = capabilities.maxImageExtent.height;
    }
    
    u32 imageCount = IMAGE_COUNT;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    if (imageCount < capabilities.minImageCount) {
        imageCount = capabilities.minImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = selectedFormat.format;
    createInfo.imageColorSpace = selectedFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = selectedPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    info("Creating swapchain with format %d, extent %dx%d, presentMode %d", 
          selectedFormat.format, extent.width, extent.height, selectedPresentMode);
    
    VkResult result = vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain);
    if (result != VKS) { fail("Failed to create swapchain! Error: %d", result); }
    
    u32 count;
    vkGetSwapchainImagesKHR(device, swapChain, &count, NULL);
    if (count > IMAGE_COUNT) { count = IMAGE_COUNT; }
    vkGetSwapchainImagesKHR(device, swapChain, &count, swapChainImages);
    
    swapChainImageFormat = createInfo.imageFormat;
    swapChainExtent = createInfo.imageExtent;
    
    info("Swapchain created successfully with %d images", count);
}
  
  
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 miplevels)
{
    debug("Enter func createImageView");
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = miplevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
  
    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, NULL, &imageView) != VKS) { fail("Failed to create image view!"); }
    return imageView;
}
  
void createImageViews()
{
    for (size_t i = 0; i < IMAGE_COUNT; i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}
  
void createRenderPass()
{
    debug("Enter func createRenderPass");
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
  
    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VKS) {
        fail("Failed to create render pass!");
    }
}
  
void createFramebuffers() {
    for (size_t i = 0; i < IMAGE_COUNT; i++) {
        VkImageView attachments[] = { swapChainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = (u32) (sizeof(attachments) / sizeof(attachments[0]));
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;
  
        if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[i]) != VKS) {
            fail("Failed to create framebuffer!");
        }
    }    
}

VkShaderModule createShaderModule(const u32 *code, u32 size)
{
    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = code;
    VkShaderModule shaderModule;

    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VKS) {
        fail("Failed to create shader module!");
    }
    return shaderModule;
}

void createGraphicsPipeline()
{
    VkShaderModule vert_shader_m = createShaderModule(vert_spv, vert_size);
    VkShaderModule frag_shader_m = createShaderModule(frag_spv, frag_size);
    
    debug("Shader modules created");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vert_shader_m;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = frag_shader_m;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = NULL;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = NULL;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)swapChainExtent.width;
    viewport.height = (f32)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = (VkOffset2D) {0, 0};
    scissor.extent = swapChainExtent;
    
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;



    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    /*
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    
    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    */

    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = NULL;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = NULL;
    
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VKS)
        fail("failed to create pipeline layout!");
    
    debug("Pipeline layout created");
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    //pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    
    debug("Calling vkCreateGraphicsPipelines");
    
    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline);
    if (result != VKS) {
        fail("failed to create graphics pipeline! Error: %d", result);
    }
    debug("Graphics pipeline created successfully");
    
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyShaderModule(device, vert_shader_m, NULL);
    vkDestroyShaderModule(device, frag_shader_m, NULL);
}

void createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = DeviceQueueIndex;
    if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VKS) {
      fail("Failed to create command pool!");
    }
}

void createCommandBuffers()
{
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = IMAGE_COUNT;
    
    VkResult result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers);
    if (result != VKS) {
      fail("failed to allocate command buffers! Error: %d\n", result);
    }
}

void createSyncObjects()
{
    debug("Enter func createSyncObjects");
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < IMAGE_COUNT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphores[i]) != VKS ||
            vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphores[i]) != VKS ||
            vkCreateFence(device, &fenceInfo, NULL, &inFlightFences[i]) != VKS) {
            fail("Failed to create synchronization objects!");
        }
    }
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VKS) {
        fail("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D) {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearValues[1] = {};
    clearValues[0].color = (VkClearColorValue) {{0.0f, 0.0f, 1.0f, 1.0f}};

    renderPassInfo.clearValueCount = (uint32_t) (sizeof(clearValues) / sizeof(clearValues[0]));
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
      vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VKS) {
        fail("failed to record command buffer!");
    }
}

void drawFrame()
{
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    u32 imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, 
                                            imageAvailableSemaphores[currentFrame], 
                                            VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { return; }
    
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkQueueSubmit(queue, 1, &submitInfo, inFlightFences[currentFrame]);
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    vkQueuePresentKHR(queue, &presentInfo);
    
    currentFrame = (currentFrame + 1) % IMAGE_COUNT;
}


void initVulkan()
{
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    info("Vulkan initialized successfully");
}

void cleanup()
{
    for (size_t i = 0; i < IMAGE_COUNT; i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
        vkDestroyImageView(device, swapChainImageViews[i], NULL);
    }
    vkDestroyRenderPass(device, renderPass, NULL);
    vkDestroySwapchainKHR(device, swapChain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
}
