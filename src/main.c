/* main.c */

#include "defines.h"
#include "callback.c"
#include "vulkan.c"

f64 target_fps = 165.000;

void fps_limiter(void)
{
    static f64 time_p = 0;
    f64 time_n = glfwGetTime();
    f64 time_d = time_n - time_p;
    if (time_d < (1.0 / target_fps)) {
      i64 sleep = (i64)(((1.0 / target_fps) - time_d) * 1e9);
      struct timespec ts = {0, sleep};
      nanosleep(&ts, NULL);
    }
    time_p = glfwGetTime();
}

void main_loop(void)
{
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        draw_frame();
    }
    vkDeviceWaitIdle(ctx.device);
}

i32 main(i32 argc, char **argv)
{
    init_window();
    MouseState state = {0.0, 0.0};
    glfwSetWindowUserPointer(ctx.window, &state);
    glfwSetKeyCallback(ctx.window, key_callback);
    glfwSetCursorPosCallback(ctx.window, cursor_pos_callback);
    glfwSetMouseButtonCallback(ctx.window, mouse_button_callback);
    glfwSetScrollCallback(ctx.window, scroll_callback);
    init_vulkan();
    main_loop();
    cleanup(); 
}
