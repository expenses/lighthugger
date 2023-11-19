cmake_minimum_required(VERSION 3.20)

project(vkcpptest)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
add_subdirectory(external/VulkanMemoryAllocator-Hpp)

add_compile_definitions(vkcpptest TRACY_ENABLE IMGUI_DEFINE_MATH_OPERATORS)
file(GLOB_RECURSE SRC_FILES src/**.cpp)
add_executable(vkcpptest ${SRC_FILES})

target_sources(vkcpptest PRIVATE
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

target_include_directories(vkcpptest PRIVATE src external/simple_vulkan_synchronization external/VulkanMemoryAllocator-Hpp/include external/tinyobjloader external/dbg-macro external/tracy/public external/imgui)
target_link_libraries(vkcpptest glfw Vulkan::Vulkan)
target_precompile_headers(vkcpptest PRIVATE src/pch.h)
target_compile_features(vkcpptest PUBLIC cxx_std_20)
target_compile_options(vkcpptest PUBLIC -Wall -Wextra -Wpedantic -Wfatal-errors -fdiagnostics-color=always)
