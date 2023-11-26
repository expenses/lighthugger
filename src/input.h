
struct CameraParams {
    glm::vec3 position;
    glm::vec3 velocity = glm::vec3(0.0);
    float fov;

    float yaw = 0.0;
    float pitch = 0.0;

    float sun_latitude;
    float sun_longitude;

    glm::vec3 facing() {
        return glm::vec3(
            cosf(yaw) * cosf(pitch),
            sinf(pitch),
            sinf(yaw) * cosf(pitch)
        );
    }

    glm::vec3 right() {
        return glm::vec3(-sinf(yaw), 0.0, cosf(yaw));
    }

    glm::vec3 sun_dir() {
        return glm::vec3(
            cosf(sun_latitude) * cosf(sun_longitude),
            sinf(sun_longitude),
            sinf(sun_latitude) * cosf(sun_longitude)
        );
    }

    void move_camera(glm::ivec3 movement_vector) {
        if (movement_vector == glm::ivec3(0)) {
            return;
        }

        auto movement = glm::normalize(glm::vec3(movement_vector)) * 0.25f;

        position += movement.x * right();
        position += movement.z * facing();
        position.y += movement.y;
    }
};

struct KeyboardState {
    bool left;
    bool right;
    bool up;
    bool down;
    bool w;
    bool a;
    bool s;
    bool d;
    bool shift;
    bool control;
    bool grab_toggled;
};

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
