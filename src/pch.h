#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define DBG_MACRO_NO_WARNING
#include <dbg.h>
#include <thsvs_simpler_vulkan_synchronization.h>
#include <tiny_obj_loader.h>

#include <cmath>
#include <fstream>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/reciprocal.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include <limits>
#include <optional>
