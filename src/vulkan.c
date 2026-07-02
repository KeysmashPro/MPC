/* Vulkan.c */

#include "defines.h"
#include "callback.h"
#include "vulkan.h"

#include "shaders/shaderdump.h"
#include "init_vulkan.c"


/* variables */

void create_swap_chain(i32 width, i32 height)
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult capResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &capabilities);
    if (capResult != VKS) { fail("Failed to get surface capabilities! Error: %d", capResult); }
    
    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, NULL);
    if (!formatCount) { fail("No surface formats found!"); }
    VkSurfaceFormatKHR* formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, formats);
    
    VkSurfaceFormatKHR selectedFormat = {PREFERRED_COLOR_FORMAT, PREFERRED_COLOR_SPACE};
    
    i32 formatFound = 0;
    for (u32 i = 0; i < formatCount; i++) {
        if (formats[i].format == PREFERRED_COLOR_FORMAT && formats[i].colorSpace == PREFERRED_COLOR_SPACE) {
            selectedFormat = formats[i];
            formatFound = 1;
            break;
        }
    }
    if (!formatFound) { selectedFormat = formats[0]; }
    free(formats);
    
    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice, ctx.surface, &presentModeCount, NULL);
    if (presentModeCount == 0) { fail("No present modes found!"); }
    
    VkPresentModeKHR* presentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice, ctx.surface, &presentModeCount, presentModes);
    
    VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    free(presentModes);
    
    VkExtent2D extent = {W_WIDTH, W_HEIGHT};
    if (capabilities.currentExtent.width != 0xFFFFFFFF) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = (u32)width;
        extent.height = (u32)height;
        if (extent.width < capabilities.minImageExtent.width)   { extent.width = capabilities.minImageExtent.width;   }
        if (extent.width > capabilities.maxImageExtent.width)   { extent.width = capabilities.maxImageExtent.width;   }
        if (extent.height < capabilities.minImageExtent.height) { extent.height = capabilities.minImageExtent.height; }
        if (extent.height > capabilities.maxImageExtent.height) { extent.height = capabilities.maxImageExtent.height; }
    }
    
    u32 imageCount = VK_IMAGE_COUNT;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    if (imageCount < capabilities.minImageCount) {
        imageCount = capabilities.minImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = ctx.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = selectedFormat.format;
    createInfo.imageColorSpace = selectedFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = selectedPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    
    VkResult result = vkCreateSwapchainKHR(ctx.device, &createInfo, NULL, &ctx.swapChain);
    if (result != VKS) { fail("Failed to create swapchain! Error: %d", result); }
    /* Previous realisation
    u32 count;
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, &count, NULL);
    if (count > VK_IMAGE_COUNT) { count = VK_IMAGE_COUNT; }
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, &count, ctx.swapChainImages);
    
    ctx.swapChainImageFormat = createInfo.imageFormat;
    ctx.swapChainExtent = createInfo.imageExtent;
    */

    u32 count;
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, &count, NULL);
    
    if (count > MAX_SWAPCHAIN_IMAGES) { fail("Swapchain has too many images!"); }
    
    ctx.swapChainImageCount = count;
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapChain, &count, ctx.swapChainImages);
    
    ctx.swapChainImageFormat = createInfo.imageFormat;
    ctx.swapChainExtent = createInfo.imageExtent;
}
  
void create_image_views()
{
    for (size_t i = 0; i < ctx.swapChainImageCount; i++) {
        ctx.swapChainImageViews[i] = create_image_view(ctx.swapChainImages[i], ctx.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}
  
void create_render_pass()
{
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = ctx.swapChainImageFormat;
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
  
    if (vkCreateRenderPass(ctx.device, &renderPassInfo, NULL, &ctx.renderPass) != VKS) {
        fail("Failed to create render pass!");
    }
}
 
void create_framebuffers()
{
    for (size_t i = 0; i < ctx.swapChainImageCount; i++) {
        VkImageView attachments[] = { ctx.swapChainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = ctx.renderPass;
        framebufferInfo.attachmentCount = (u32) (sizeof(attachments) / sizeof(attachments[0]));
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = ctx.swapChainExtent.width;
        framebufferInfo.height = ctx.swapChainExtent.height;
        framebufferInfo.layers = 1;
  
        if (vkCreateFramebuffer(ctx.device, &framebufferInfo, NULL, &ctx.swapChainFramebuffers[i]) != VKS) {
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

    if (vkCreateShaderModule(ctx.device, &createInfo, NULL, &shaderModule) != VKS) {
        fail("Failed to create shader module!");
    }
    return shaderModule;
}

void create_graphics_pipeline()
{
    VkShaderModule vert_shader_m = createShaderModule(vert_spv, vert_size);
    VkShaderModule frag_shader_m = createShaderModule(frag_spv, frag_size);
    
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
    vertexInputInfo.pVertexAttributeDescriptions = NULL;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)ctx.swapChainExtent.width;
    viewport.height = (f32)ctx.swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = (VkOffset2D) {0, 0};
    scissor.extent = ctx.swapChainExtent;
    
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
    
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    
    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = NULL;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = NULL;
    
    if (vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VKS) {
        fail("failed to create pipeline layout!");
    }
    
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
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = ctx.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    
    VkResult result = vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx.graphicsPipeline);
    if (result != VKS) {
        fail("failed to create graphics pipeline! Error: %d", result);
    }
    vkDestroyPipelineLayout(ctx.device, pipelineLayout, NULL);
    vkDestroyShaderModule(ctx.device, vert_shader_m, NULL);
    vkDestroyShaderModule(ctx.device, frag_shader_m, NULL);
}

void create_command_buffers()
{
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = ctx.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = VK_IMAGE_COUNT;
    
    VkResult result = vkAllocateCommandBuffers(ctx.device, &allocInfo, ctx.commandBuffers);
    if (result != VKS) {
      fail("failed to allocate command buffers! Error: %d\n", result);
    }
}


void record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VKS) {
        fail("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = ctx.renderPass;
    renderPassInfo.framebuffer = ctx.swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D) {0, 0};
    renderPassInfo.renderArea.extent = ctx.swapChainExtent;

    VkClearValue clearValues[1] = {};
    clearValues[0].color = (VkClearColorValue) {{0.0f, 0.0f, 1.0f, 1.0f}};

    renderPassInfo.clearValueCount = (uint32_t) (sizeof(clearValues) / sizeof(clearValues[0]));
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.graphicsPipeline);
    
    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)ctx.swapChainExtent.width;
    viewport.height = (f32)ctx.swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = ctx.swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VKS) {
        fail("failed to record command buffer!");
    }
}

void draw_frame()
{
    vkWaitForFences(ctx.device, 1, &ctx.inFlightFences[times.i_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(ctx.device, 1, &ctx.inFlightFences[times.i_frame]);
    times.prev = times.curr;
    times.curr = glfwGetTime();
    times.diff = times.curr - times.prev;
    
    u32 imageIndex;
    VkResult result = vkAcquireNextImageKHR(ctx.device, ctx.swapChain, UINT64_MAX, 
                                            ctx.imageAvailableSemaphores[times.i_frame], 
                                            VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR ) { 
        handle_window_resize();
        return;
    }
    
    vkResetCommandBuffer(ctx.commandBuffers[times.i_frame], 0);
    record_command_buffer(ctx.commandBuffers[times.i_frame], imageIndex);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {ctx.imageAvailableSemaphores[times.i_frame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &ctx.commandBuffers[times.i_frame];
    
    VkSemaphore signalSemaphores[] = {ctx.renderFinishedSemaphores[times.i_frame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkQueueSubmit(ctx.queue, 1, &submitInfo, ctx.inFlightFences[times.i_frame]);
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {ctx.swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(ctx.queue, &presentInfo);
     if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        window_resize = 1;
        handle_window_resize();
     }
    
    if (window_resize) { handle_window_resize(); }
    times.i_frame = (times.i_frame + 1) % VK_IMAGE_COUNT;
}

void init_vulkan()
{
    create_instance();
    create_surface();
    pick_physical_device();
    create_logical_device();

    i32 width, height;
    glfwGetFramebufferSize(ctx.window, &width, &height);
    create_swap_chain(width, height);

    create_image_views();
    create_render_pass();
    create_framebuffers();
    create_graphics_pipeline();
    create_command_pool();
    create_command_buffers();
    create_sync_objects();

    info("Vulkan initialized successfully");
    times.prev = glfwGetTime();
}

void cleanup_swap_chain()
{
    iterate(i, ctx.swapChainImageCount) {
            vkDestroyFramebuffer(ctx.device, ctx.swapChainFramebuffers[i], NULL);
        vkDestroyImageView(ctx.device, ctx.swapChainImageViews[i], NULL);
    }
    vkDestroySwapchainKHR(ctx.device, ctx.swapChain, NULL);
}

void cleanup()
{
    cleanup_swap_chain();
    vkDestroyPipeline(ctx.device, ctx.graphicsPipeline, NULL);
    vkDestroyCommandPool(ctx.device, ctx.commandPool, NULL);
    
    iterate(i, VK_IMAGE_COUNT) {
        vkDestroySemaphore(ctx.device, ctx.imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(ctx.device, ctx.renderFinishedSemaphores[i], NULL);
        vkDestroyFence(ctx.device, ctx.inFlightFences[i], NULL);
    }

    vkDestroyRenderPass(ctx.device, ctx.renderPass, NULL);
    vkDestroyDevice(ctx.device, NULL);
    vkDestroySurfaceKHR(ctx.instance, ctx.surface, NULL);
    vkDestroyInstance(ctx.instance, NULL);
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
}

void handle_window_resize()
{
    i32 width = 0, height = 0;
    glfwGetFramebufferSize(ctx.window, &width, &height);
    
    while (!width + !height) {
        glfwGetFramebufferSize(ctx.window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(ctx.device);

    cleanup_swap_chain();
    create_swap_chain(width, height);
    create_image_views();
    create_framebuffers();
    window_resize = 0;
}
