#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_truetype.h"

#include "./defines.h"
#include "./vulkan.c"
#include "./input.c"

f64 target_fps = 165.000;

void fps_limiter(void)
{
    f64 time_p = 0;
    f64 time_n = glfwGetTime();
    f64 time_d = time_n - time_p;
    if (time_d < (1.0 / target_fps)) {
      i64 sleep = (i64)(((1.0 / target_fps) - time_d) * 1e9);
      struct timespec ts = {0, sleep};
      nanosleep(&ts, NULL);
    }
    time_p = glfwGetTime();
}


void mainLoop()
{
    debug("Enter func mainLoop");
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
        fps_limiter();
    }
    vkDeviceWaitIdle(device);
}


i32 main(i32 argc, char **argv)
{
    initWindow();
    InputState state = {0.0, 0.0};
    glfwSetWindowUserPointer(window, &state);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    initVulkan();
    mainLoop();
    cleanup(); 
}
