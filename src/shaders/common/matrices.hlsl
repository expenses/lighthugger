// Taken from https://www.geertarien.com/blog/2017/07/30/breakdown-of-the-lookAt-function-in-OpenGL/
float4x4 lookAt(float3 eye, float3 at, float3 up) {
    float3 zaxis = -normalize(at - eye);
    float3 xaxis = normalize(cross(zaxis, up));
    float3 yaxis = cross(xaxis, zaxis);

    return transpose(float4x4(
        float4(xaxis.x, xaxis.y, xaxis.z, -dot(xaxis, eye)),
        float4(yaxis.x, yaxis.y, yaxis.z, -dot(yaxis, eye)),
        float4(zaxis.x, zaxis.y, zaxis.z, -dot(zaxis, eye)),
        float4(0, 0, 0, 1)
    ));
}

float4x4 OrthographicProjection(
    float left,
    float bottom,
    float right,
    float top,
    float near,
    float far
) {
    return transpose(float4x4(
        float4(2.0f / (right - left), 0.0f, 0.0f, 0.0f),
        float4(0.0f, 2.0f / (bottom - top), 0.0f, 0.0f),
        float4(0.0f, 0.0f, 1.0f / (near - far), 0.0f),
        float4(
            -(right + left) / (right - left),
            -(bottom + top) / (bottom - top),
            near / (near - far),
            1.0f
        )
    ));
}
