// Implementations copied from https://github.com/fu5ha/ultraviolet.

// In GLSL matrices are cols major or w/e, but not in HLSL.
float4x4
create_matrix_from_cols_glsl(float4 c0, float4 c1, float4 c2, float4 c3) {
    return float4x4(c0, c1, c2, c3);
}

float4x4 lookAt(float3 eye, float3 at, float3 up) {
    float3 f = normalize(at - eye);
    float3 r = normalize(cross(f, up));
    float3 u = cross(r, f);
    return create_matrix_from_cols_glsl(
        float4(r.x, u.x, -f.x, 0.0),
        float4(r.y, u.y, -f.y, 0.0),
        float4(r.z, u.z, -f.z, 0.0),
        float4(-dot(r, eye), -dot(u, eye), dot(f, eye), 1.0)
    );
}

float4x4 OrthographicProjection(
    float left,
    float right,
    float bottom,
    float top,
    float near,
    float far
) {
    float rml = right - left;
    float rpl = right + left;
    float tmb = top - bottom;
    float tpb = top + bottom;
    float fmn = far - near;
    return create_matrix_from_cols_glsl(
        float4(2.0 / rml, 0.0, 0.0, 0.0),
        float4(0.0, -2.0 / tmb, 0.0, 0.0),
        float4(0.0, 0.0, -1.0 / fmn, 0.0),
        float4(-(rpl / rml), -(tpb / tmb), -(near / fmn), 1.0)
    );
}
