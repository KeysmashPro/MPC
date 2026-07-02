/* init_vulkan.c */

#include "defines.h"
#include "shaders/shaderdump.h"
#include "vulkan.h"
#include "callback.h"


/* variables */

char *WIN_NAME = "UCP";
u64 W_WIDTH  = 720;
u64 W_HEIGHT = 480;

VkFormat PREFERRED_COLOR_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
VkColorSpaceKHR PREFERRED_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

time_data times = {0.0f, 0.0f, 0.0f, 0, 0};
vk_context ctx;


void init_window()
{
    if(!glfwInit()) { fail("Failed to glfwInit!"); }
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    ctx.window = glfwCreateWindow(W_WIDTH, W_HEIGHT, WIN_NAME, NULL, NULL);
    glfwSetFramebufferSizeCallback(ctx.window, framebuffer_resize_callback);
    if (!ctx.window) { glfwTerminate(); fail("Failed to create window!"); }
    glfwShowWindow(ctx.window);
}

void create_instance()
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

    VkResult instance_result = vkCreateInstance(&createInfo, NULL, &ctx.instance);
    if (instance_result != VKS) {
        fail("Vulkan instance creation failed! Error code: %d", instance_result);
    }
}

void create_surface()
{
    VkResult res = glfwCreateWindowSurface(ctx.instance, ctx.window, NULL, &ctx.surface);
    if (res != VKS) { fail("failed to create window surface!"); }
}

void pick_physical_device()
{
    u32 deviceCount = 0;
    if (vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, NULL) != VKS) { fail("Fail physical devices"); }
    if (deviceCount == 0) { fail("Found 0 GPUs with Vulkan support!\n"); }

    VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices);
    ctx.physicalDevice = devices[0];
    free(devices);
    
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(ctx.physicalDevice, &deviceProperties);
    info("Selected GPU: %s", deviceProperties.deviceName);
}

void create_logical_device()
{
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &queueFamilyCount, queueFamilies);
    
    u32 selectedIndex = UINT32_MAX;
    for (u32 i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        VkBool32 presentSupport = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(ctx.physicalDevice, i, ctx.surface, &presentSupport);
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
    createInfo.pNext = &vulkan11Features;
    
    VkResult result = vkCreateDevice(ctx.physicalDevice, &createInfo, NULL, &ctx.device);
    if (result != VKS) { fail("Failed to create logical device! Error: %d", result); }
    vkGetDeviceQueue(ctx.device, selectedIndex, 0, &ctx.queue);
    ctx.DeviceQueueIndex = selectedIndex;
}

VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 miplevels)
{
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
    if (vkCreateImageView(ctx.device, &viewInfo, NULL, &imageView) != VKS) { fail("Failed to create image view!"); }
    return imageView;
}

void create_command_pool()
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = ctx.DeviceQueueIndex;
    if (vkCreateCommandPool(ctx.device, &poolInfo, NULL, &ctx.commandPool) != VKS) {
      fail("Failed to create command pool!");
    }
}

void create_sync_objects()
{
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < VK_IMAGE_COUNT; i++) {
        if (vkCreateSemaphore(ctx.device, &semaphoreInfo, NULL, &ctx.imageAvailableSemaphores[i]) != VKS ||
            vkCreateSemaphore(ctx.device, &semaphoreInfo, NULL, &ctx.renderFinishedSemaphores[i]) != VKS ||
            vkCreateFence(ctx.device, &fenceInfo, NULL, &ctx.inFlightFences[i]) != VKS) {
            fail("Failed to create synchronization objects!");
        }
    }
}


