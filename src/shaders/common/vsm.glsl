// From https://therealmjp.github.io/posts/shadow-maps/
// https://github.com/TheRealMJP/Shadows

float Linstep(float a, float b, float v) {
    return saturate((v - a) / (b - a));
}

// Reduces VSM light bleedning
float ReduceLightBleeding(float pMax, float amount) {
  // Remove the [0, amount] tail and linearly rescale (amount, 1].
   return Linstep(amount, 1.0f, pMax);
}

float ChebyshevUpperBound(float2 moments, float mean, float minVariance,
                          float lightBleedingReduction) {
    // Compute variance
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, minVariance);

    // Compute probabilistic upper bound
    float d = mean - moments.x;
    float pMax = variance / (variance + (d * d));

    pMax = ReduceLightBleeding(pMax, lightBleedingReduction);

    // One-tailed Chebyshev
    return (mean <= moments.x ? 1.0f : pMax);
}

float SampleShadowMapVSM(in float3 shadowPos, uint cascadeIdx) {
    float depth = shadowPos.z;

    float d = textureLod(sampler2DArray(shadowmap, clamp_sampler), float3(shadowPos.xy, cascadeIdx),
                                           0).x;
    float2 occluder = float2(d, d * d);

    float LightBleedingReduction = 0.0;
    float VSMBias = uniforms.vsm_bias / 100000.0 / uniforms.shadow_cam_distance;

    return ChebyshevUpperBound(occluder, depth, VSMBias, LightBleedingReduction);
}
