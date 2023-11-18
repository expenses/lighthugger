// Adapted from https://vincent-p.github.io/posts/vulkan_perspective_matrix/
// (simplified and transposed)
glm::mat4 infinite_reverse_z_perspective(
    float fov_rad,
    float width,
    float height,
    float near
) {
    float focal_length = 1.0f / std::tan(fov_rad / 2.0f);

    float x = focal_length / (width / height);
    float y = -focal_length;

    return glm::mat4(
        glm::vec4(x, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, y, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, -1.0f),
        glm::vec4(0.0f, 0.0f, near, 0.0f)
    );
}

glm::mat4 reverse_z_perspective(
    float fov_rad,
    float width,
    float height,
    float near,
    float far
) {
    float focal_length = 1.0f / std::tan(fov_rad / 2.0f);

    float x = focal_length / (width / height);
    float y = -focal_length;
    float A = near / (far - near);
    float B = far * A;

    return glm::mat4(
        glm::vec4(x, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, y, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, A, -1.0f),
        glm::vec4(0.0f, 0.0f, B, 0.0f)
    );

    /*if (inverse)
    {
        *inverse = float4x4({
            1/x,  0.0f, 0.0f,  0.0f,
            0.0f,  1/y, 0.0f,  0.0f,
            0.0f, 0.0f, 0.0f, 1/B,
            0.0f, 0.0f,  -1.0f,   A/B,
        });
    }*/
}
