cmake_minimum_required(VERSION 3.20)

project(vkcpptest)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
add_subdirectory(VulkanMemoryAllocator-Hpp)

file(GLOB SRC_FILES src/*.cpp)
add_executable(vkcpptest ${SRC_FILES})

target_include_directories(vkcpptest PRIVATE src simple_vulkan_synchronization VulkanMemoryAllocator-Hpp/include)
target_link_libraries(vkcpptest glfw Vulkan::Vulkan)
target_precompile_headers(vkcpptest PRIVATE src/pch.h)
target_compile_features(vkcpptest PUBLIC cxx_std_20)
target_compile_options(vkcpptest PUBLIC -Wall -Wextra -Wpedantic -Wfatal-errors -fdiagnostics-color=always)
