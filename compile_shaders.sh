for file in src/shaders/*.comp; do
    glslc -std=450 --target-env=vulkan1.3 --target-spv=spv1.6 $file -o compiled_shaders/$(basename $file .comp).spv -I src || exit 1
done

error_flags="-Wall -Wextra -Weverything -Wpedantic -Werror -Wno-c++98-compat -Wno-unused-macros -Wno-missing-variable-declarations -Wno-missing-prototypes -Wno-shorten-64-to-32 -Wno-float-conversion -Wno-global-constructors"

compiler="dxc -HV 2021 $error_flags -I src -spirv -fspv-target-env=vulkan1.2 -T lib_6_7 -enable-16bit-types -fvk-use-scalar-layout"

for file in src/shaders/*.hlsl; do
    $compiler $file -Fo compiled_shaders/$(basename $file .hlsl).spv || exit 1
done

mkdir -p compiled_shaders/visbuffer_rasterization

for file in src/shaders/visbuffer_rasterization/*.hlsl; do
    $compiler $file -Fo compiled_shaders/visbuffer_rasterization/$(basename $file .hlsl).spv || exit 1
done

mkdir -p compiled_shaders/compute

for file in src/shaders/compute/*.hlsl; do
    $compiler $file -Fo compiled_shaders/compute/$(basename $file .hlsl).spv || exit 1
done
