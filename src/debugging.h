
VKAPI_ATTR VkBool32 VKAPI_CALL debug_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* /*pUserData*/
) {
    std::cout
        << "["
        << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(
               messageSeverity
           ))
        << "]["
        << vk::to_string(
               static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)
           )
        << "][" << pCallbackData->pMessageIdName << "]\t"
        << pCallbackData->pMessage << std::endl;

    return false;
}
