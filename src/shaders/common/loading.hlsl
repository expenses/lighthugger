#pragma once
#include <shared_cpu_gpu.h>

template<class T>
T load_value(uint64_t address, uint32_t offset) {
    return vk::RawBufferLoad<T>(address + sizeof(T) * offset);
}

template<class T>
T load_value_and_increment_address(inout uint64_t address) {
    uint64_t load_address = address;
    // Update the address by sizeof(T) to help with the loading functions below.
    address += sizeof(T);
    return vk::RawBufferLoad<T>(load_address);
}

MeshInfo load_mesh_info(uint64_t address) {
    MeshInfo info;
    info.positions = load_value_and_increment_address<uint64_t>(address);
    info.indices = load_value_and_increment_address<uint64_t>(address);
    info.normals = load_value_and_increment_address<uint64_t>(address);
    info.uvs = load_value_and_increment_address<uint64_t>(address);
    info.num_indices = load_value_and_increment_address<uint32_t>(address);
    info.num_vertices = load_value_and_increment_address<uint32_t>(address);
    info.flags = load_value_and_increment_address<uint32_t>(address);
    info.bounding_sphere_radius = load_value_and_increment_address<float>(address);
    info.texture_scale = load_value_and_increment_address<float2>(address);
    info.texture_offset = load_value_and_increment_address<float2>(address);
    info.albedo_texture_index = load_value_and_increment_address<uint32_t>(address);
    info.metallic_roughness_texture_index = load_value_and_increment_address<uint32_t>(address);
    info.normal_texture_index = load_value_and_increment_address<uint32_t>(address);
    info.albedo_factor = load_value_and_increment_address<float3>(address);
    return info;
}

template<class T>
T load_value_and_increment_offset(ByteAddressBuffer byte_address_buffer, inout uint32_t offset) {
    uint32_t load_offset = offset;
    // Update the offset by sizeof(T) to help with the loading functions below.
    offset += sizeof(T);
    return byte_address_buffer.Load<T>(load_offset);
}

template<class T>
T load_value_and_increment_offset(RWByteAddressBuffer byte_address_buffer, inout uint32_t offset) {
    uint32_t load_offset = offset;
    // Update the offset by sizeof(T) to help with the loading functions below.
    offset += sizeof(T);
    return byte_address_buffer.Load<T>(load_offset);
}

uint16_t3 load_uint16_t3(uint64_t address, uint32_t offset) {
    // The alignment of 2 is necessary here, otherwise it aligns the read to 4
    // which is wrong. For whatever reason, you can do 3 loads of `uint16_t`s
    // without specifying the alignment and it works fine.
    return vk::RawBufferLoad<uint16_t3>(address + sizeof(uint16_t3) * offset, 2);
}
