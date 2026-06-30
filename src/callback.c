/* INPUT CALLBACKS */

#include "base.h"

typedef struct {
  f64 mouse_x;
  f64 mouse_y;
  f64 scroll_x;
  f64 scroll_y;
  u8  buttons;
} MouseState;

u8 window_resize = 0;
void framebuffer_resize_callback(GLFWwindow* window, i32 width, i32 height) {
    window_resize = 1;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
      switch(key) {
        case GLFW_KEY_Q:  glfwSetWindowShouldClose(window, GLFW_TRUE); break;
      }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    MouseState* state = (MouseState*)glfwGetWindowUserPointer(window);
    switch(button) {
      case GLFW_MOUSE_BUTTON_LEFT:   state->buttons = (action == GLFW_PRESS) ? state->buttons | (1 << 0) : state->buttons & ~(1 << 0); break;
      case GLFW_MOUSE_BUTTON_RIGHT:  state->buttons = (action == GLFW_PRESS) ? state->buttons | (1 << 1) : state->buttons & ~(1 << 1); break;
      case GLFW_MOUSE_BUTTON_MIDDLE: state->buttons = (action == GLFW_PRESS) ? state->buttons | (1 << 2) : state->buttons & ~(1 << 2); break;
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    MouseState* state = (MouseState*)glfwGetWindowUserPointer(window);
    state->mouse_x = xpos;
    state->mouse_y = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    MouseState* state = (MouseState*)glfwGetWindowUserPointer(window);
    state->scroll_x = xoffset;
    state->scroll_y = yoffset;
}

