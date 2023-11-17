// Taken from https://gist.github.com/pezcode/1609b61a1eedd207ec8c5acf6f94f53a.

// these matrices are for left-handed coordinate systems, with depth mapped to [1;0]
// the derivation for other matrices is analogous

// to get a perspective matrix with reversed z, simply swap the near and far plane
glm::mat4 perspectiveFovReverseZLH_ZO(
    float fov,
    float width,
    float height,
    float zNear,
    float zFar
) {
    return glm::perspectiveFovLH_ZO(fov, width, height, zFar, zNear);
};

// now let zFar go towards infinity
glm::mat4 infinitePerspectiveFovReverseZLH_ZO(
    float fov,
    float width,
    float height,
    float zNear
) {
    const float h = glm::cot(0.5f * fov);
    const float w = h * height / width;
    glm::mat4 result = glm::zero<glm::mat4>();
    result[0][0] = w;
    result[1][1] = h;
    result[2][2] = 0.0f;
    result[2][3] = 1.0f;
    result[3][2] = zNear;
    return result;
};
