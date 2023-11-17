dxc src/shaders/display_transform.hlsl -HV 2021 -Werror -Wall -spirv -T lib_6_7 -Fo compiled_shaders/display_transform.spv
dxc src/shaders/fullscreen_tri.hlsl -HV 2021 -Werror -Wall -spirv -T lib_6_7 -Fo compiled_shaders/fullscreen_tri.spv
dxc src/shaders/render_geometry.hlsl -HV 2021 -Werror -Wall -spirv -T lib_6_7 -Fo compiled_shaders/render_geometry.spv
