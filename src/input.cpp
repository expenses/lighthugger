#include "input.h"

glm::vec3 clamp_length(glm::vec3 vector, float max) {
    auto length_squared = glm::length2(vector);
    if (length_squared > max * max) {
        return vector / sqrtf(length_squared) * max;
    } else {
        return vector;
    }
}

glm::vec2 clamp_length(glm::vec2 vector, float max) {
    auto length_squared = glm::length2(vector);
    if (length_squared > max * max) {
        return vector / sqrtf(length_squared) * max;
    } else {
        return vector;
    }
}

void CameraParams::update(glm::ivec3 movement_vector, glm::ivec2 sun_vector) {
    auto accel = 0.1f;

    if (movement_vector != glm::ivec3(0)) {
        auto normalized_movement =
            glm::normalize(glm::vec3(movement_vector)) * accel;
        velocity += normalized_movement.z * facing()
            + normalized_movement.x * right()
            + normalized_movement.y * glm::vec3(0, 1, 0);
        velocity = clamp_length(velocity, 0.75);
    } else {
        velocity *= (1.0 - accel);
    }

    if (sun_vector != glm::ivec2(0)) {
        sun_delta += glm::normalize(glm::vec2(sun_vector)) * 0.001f;
        sun_delta = clamp_length(sun_delta, 0.05f);
    } else {
        sun_delta *= 0.9;
    }

    position += velocity;

    sun_latitude += sun_delta.x;
    sun_longitude += sun_delta.y;
    sun_longitude =
        std::clamp(sun_longitude, 0.0f, std::numbers::pi_v<float> / 2.0f);
}

void glfw_key_callback(
    GLFWwindow* window,
    int key,
    int /*scancode*/,
    int action,
    int /*mods*/
) {
    KeyboardState& keyboard_state =
        *static_cast<KeyboardState*>(glfwGetWindowUserPointer(window));
    switch (key) {
        case GLFW_KEY_LEFT:
            keyboard_state.left = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_RIGHT:
            keyboard_state.right = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_UP:
            keyboard_state.up = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_DOWN:
            keyboard_state.down = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_W:
            keyboard_state.w = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_A:
            keyboard_state.a = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_S:
            keyboard_state.s = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_D:
            keyboard_state.d = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_LEFT_SHIFT:
            keyboard_state.shift = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_LEFT_CONTROL:
            keyboard_state.control = action != GLFW_RELEASE;
            break;
        case GLFW_KEY_G:
            keyboard_state.grab_toggled ^= (action == GLFW_PRESS);
            if (keyboard_state.grab_toggled) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            break;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}
