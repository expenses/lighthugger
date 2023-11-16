dxc src/shaders/blit_srgb.hlsl -spirv -T vs_6_0 -E VSMain -Fo compiled_shaders/blit_vs.spv
dxc src/shaders/blit_srgb.hlsl -spirv -T ps_6_0 -E PSMain -Fo compiled_shaders/blit_ps.spv
