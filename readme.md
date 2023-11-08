A fairly simple Vulkan with C++ and glFW starter project

- Based on the Vulkan-Hpp wrapper bindings
- Uses `VK_KHR_dynamic_rendering` to avoid having to deal with `VkFramebuffer` and `VkRenderPass` objects
- Uses https://github.com/Tobski/simple_vulkan_synchronization for more reasonable pipeline barrier syncronisation
- Uses https://github.com/charles-lunarg/vk-bootstrap to reduce the initial setup boilerplate. Might remove it in the future!
