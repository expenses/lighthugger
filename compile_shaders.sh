shaders='
display_transform.hlsl
fullscreen_tri.hlsl
render_geometry.hlsl
read_depth.hlsl
write_draw_calls.hlsl
'

error_flags='-Wall -Wextra -Weverything -Wpedantic -Werror -Wno-c++98-compat -Wno-unused-macros -Wno-missing-variable-declarations -Wno-missing-prototypes'

for file in $shaders; do
    dxc -HV 2021 $error_flags -spirv -fspv-target-env=vulkan1.1 -T lib_6_7 src/shaders/$file -Fo compiled_shaders/$(basename $file .hlsl).spv || exit 1
done
