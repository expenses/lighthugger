
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

    void update(glm::ivec3 movement_vector, glm::ivec2 sun_vector);
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
);
