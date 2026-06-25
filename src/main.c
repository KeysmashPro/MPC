#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_truetype.h"

#include "./defines.h"
#include "./vulkan.c"
#include "./input.c"

const f64 TFPS = 125.300;

static inline void fps_limiter(void) {
  static f64 last_fps_time = 0;
  f64 current_fps_time = glfwGetTime();
  if (current_fps_time - last_fps_time < (1.0/TFPS))
    usleep(((1.0/TFPS) - (current_fps_time - last_fps_time)) * 1e6);
  last_fps_time = glfwGetTime();
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
