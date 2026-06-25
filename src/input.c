// INPUT CALLBACKS

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "./defines.h"

typedef struct {
  f64 mouse_x;
  f64 mouse_y;
  f64 scroll_x;
  f64 scroll_y;
  u8  buttons;
} InputState;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
      switch(key) {
        case GLFW_KEY_Q:  glfwSetWindowShouldClose(window, GLFW_TRUE); break;
      }
    } else if (action == GLFW_RELEASE) {
      switch(key) {
        case GLFW_KEY_0: break;
      }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    InputState* state = (InputState*)glfwGetWindowUserPointer(window);
    switch(button) {
      case GLFW_MOUSE_BUTTON_LEFT:   state->buttons = (action == GLFW_PRESS) ? state->buttons | (1 << 0) : state->buttons & ~(1 << 0); break;
      case GLFW_MOUSE_BUTTON_RIGHT:  state->buttons = (action == GLFW_PRESS) ? state->buttons | (1 << 1) : state->buttons & ~(1 << 1); break;
      case GLFW_MOUSE_BUTTON_MIDDLE: state->buttons = (action == GLFW_PRESS) ? state->buttons | (1 << 2) : state->buttons & ~(1 << 2); break;
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    InputState* state = (InputState*)glfwGetWindowUserPointer(window);
    state->mouse_x = xpos;
    state->mouse_y = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    InputState* state = (InputState*)glfwGetWindowUserPointer(window);
    state->scroll_x = xoffset;
    state->scroll_y = yoffset;
}
