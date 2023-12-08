cmake_minimum_required(VERSION 3.20)

project(lighthugger)

find_package(Vulkan REQUIRED)
add_subdirectory(external/VulkanMemoryAllocator-Hpp)
add_subdirectory(external/fastgltf)
add_subdirectory(external/meshoptimizer)
add_subdirectory(external/glfw)

add_compile_definitions(lighthugger TRACY_ENABLE IMGUI_DEFINE_MATH_OPERATORS)
file(GLOB_RECURSE SRC_FILES src/**.cpp)
add_executable(lighthugger ${SRC_FILES})

target_sources(lighthugger PRIVATE
    # Tracy
    "external/tracy/public/TracyClient.cpp"
    # Imgui
    # https://vkguide.dev/docs/extra-chapter/implementing_imgui/
    "external/imgui/imgui.cpp"
    "external/imgui/imgui_draw.cpp"
    "external/imgui/imgui_widgets.cpp"
    "external/imgui/imgui_tables.cpp"
    "external/imgui/imgui_demo.cpp"
    "external/imgui/backends/imgui_impl_glfw.cpp"
    "external/imgui/backends/imgui_impl_vulkan.cpp"
)

target_include_directories(lighthugger PRIVATE src external/simple_vulkan_synchronization external/VulkanMemoryAllocator-Hpp/include external/VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator/include external/dbg-macro external/tracy/public external/imgui external/meshoptimizer/src)
target_link_libraries(lighthugger glfw Vulkan::Vulkan fastgltf meshoptimizer zstd)
target_precompile_headers(lighthugger PRIVATE src/pch.h)
target_compile_features(lighthugger PUBLIC cxx_std_20)
target_compile_options(lighthugger PUBLIC -Wall -Wextra -Wpedantic -Wfatal-errors -fdiagnostics-color=always)

# Use extra warnings when compiling with clang.
target_compile_options(lighthugger PRIVATE
$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
    -Wall -Wextra -Wpedantic -Wfatal-errors -fdiagnostics-color=always -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-pre-c++20-compat-pedantic -Wno-extra-semi-stmt -Wno-missing-prototypes -Wno-double-promotion -Wno-switch-enum
    >
)
