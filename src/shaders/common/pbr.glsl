const static float PI = 3.14159265358979323846264338327950288;

float F_Schlick(float u, float f0, float f90) {
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(NoL, 1.0, f90);
    float viewScatter = F_Schlick(NoV, 1.0, f90);
    return lightScatter * viewScatter * (1.0 / PI);
}

float D_GGX(float NoH, float a) {
    float a2 = a * a;
    float f = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * f * f);
}

float3 F_Schlick(float u, float3 f0) {
    return f0 + (float3(1.0, 1.0, 1.0) - f0) * pow(1.0 - u, 5.0);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a) {
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / (GGXV + GGXL);
}

float Fd_Lambert() {
    return 1.0 / PI;
}

float reflectance_for_ior(float ior) {
    float reflectance_sqrt = (ior - 1.0) / (ior + 1.0);
    return reflectance_sqrt * reflectance_sqrt;
}

float3 diffuse_color(float3 base_color, float metallic) {
    return (1.0 - metallic) * base_color;
}

float3 BRDF(
    float3 v,
    float3 l,
    float3 n,
    float perceptualRoughness,
    float metallic,
    float3 baseColor
) {
    float3 h = normalize(v + l);

    float3 diffuseColor = diffuse_color(baseColor, metallic);

    float3 f0 =
        reflectance_for_ior(1.5) * (1.0 - metallic) + baseColor * metallic;

    float NoV = clamp(dot(n, v), 1e-5, 1.0);
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

    // perceptually linear roughness to roughness (see parameterization)
    float roughness = perceptualRoughness * perceptualRoughness;

    float D = D_GGX(NoH, roughness);
    float3 F = F_Schlick(LoH, f0);
    float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

    // specular BRDF
    float3 Fr = (D * V) * F;

    // diffuse BRDF
    float3 Fd = diffuseColor * Fd_Burley(NoV, NoL, LoH, roughness);

    float3 irradiance = (Fd + Fr) * NoL;

    // Guard against divisions by zero. Mostly from
    // `V_SmithGGXCorrelated` and (I think) backfacing geometry.
    return mix(irradiance, float3(0.0), isnan(irradiance));
}
