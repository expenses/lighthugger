#include "shared_cpu_gpu.h"

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
    bool ui_toggled = true;
};

struct CameraParams {
    glm::vec3 position;
    glm::vec3 velocity = glm::vec3(0.0);
    float fov;

    float yaw = 0.0;
    float pitch = 0.0;

    float sun_latitude;
    float sun_longitude;

    glm::vec2 sun_delta = glm::vec2(0.0);

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

    void update(KeyboardState& keyboard_state) {
        int sun_left_right =
            int(keyboard_state.right) - int(keyboard_state.left);
        int sun_up_down = int(keyboard_state.up) - int(keyboard_state.down);

        update(
            glm::ivec3(
                int(keyboard_state.d) - int(keyboard_state.a),
                int(keyboard_state.shift) - int(keyboard_state.control),
                int(keyboard_state.w) - int(keyboard_state.s)
            ),
            glm::ivec2(sun_left_right, sun_up_down)
        );
    }

    void update(glm::ivec3 movement_vector, glm::ivec2 sun_vector);

    void rotate_camera(glm::dvec2 mouse_delta) {
        pitch -= static_cast<float>(mouse_delta.y) / 1024.0f;
        yaw += static_cast<float>(mouse_delta.x) / 1024.0f;
        pitch = std::clamp(
            pitch,
            -std::numbers::pi_v<float> / 2.0f + 0.0001f,
            std::numbers::pi_v<float> / 2.0f
        );
    }
};

void glfw_key_callback(
    GLFWwindow* window,
    int key,
    int /*scancode*/,
    int action,
    int /*mods*/
);

void draw_imgui_window(
    Uniforms* uniforms,
    CameraParams& camera_params,
    KeyboardState& keyboard_state,
    bool& copy_view
);
