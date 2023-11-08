#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
// Use this small lib to make inserting command barriers easier to reason about.
#include <thsvs_simpler_vulkan_synchronization.h>

#define VMA_VULKAN_VERSION 1002000
#include "vk_mem_alloc.hpp"

#include <stdexcept>
#include <iostream>
#include <limits>
#include <cmath>
