// https://github.com/KhronosGroup/Vulkan-Hpp/blob/64539664151311b63485a42277db7ffaba1c0c63/samples/RayTracing/RayTracing.cpp#L539-L549
void check_vk_result(vk::Result err) {
  if ( err != vk::Result::eSuccess )
  {
    std::cerr << "Vulkan error " << vk::to_string(err);
    if ( err < vk::Result::eSuccess )
    {
      abort();
    }
  }
}
