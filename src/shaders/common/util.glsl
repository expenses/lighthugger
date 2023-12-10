#include "prefix_sum.glsl"

uint32_t load_index(MeshInfo mesh_info, uint32_t vertex_id) {
    if (bool(mesh_info.flags & MESH_INFO_FLAGS_32_BIT_INDICES)) {
        return Index32Buffer(mesh_info.indices).indices[vertex_id];
    } else {
        return Index16Buffer(mesh_info.indices).indices[vertex_id];
    }
}

float3
calculate_world_pos(Instance instance, MeshInfo mesh_info, uint32_t index) {
    float3 position =
        QuantizedPositionBuffer(mesh_info.positions).positions[index];
    return (instance.transform * float4(position, 1.0)).xyz;
}

uint32_t pack(uint32_t triangle_index, uint32_t instance_index) {
    return triangle_index << 24 | instance_index;
}

// Super easy.
float convert_infinite_reverze_z_depth(float depth) {
    return NEAR_PLANE / depth;
}

PrefixSumBuffer prefix_sum_buffer_for_cascade(uint32_t cascade_index) {
    return PrefixSumBuffer(
        get_uniforms().num_meshlets_prefix_sum
        + cascade_index * PREFIX_SUM_BUFFER_SECTOR_SIZE
    );
}

uint32_t total_num_meshlets_for_cascade(uint32_t cascade_index) {
    return prefix_sum_total(prefix_sum_buffer_for_cascade(cascade_index));
}

uint32_t dispatch_size(uint32_t width, uint32_t workgroup_size) {
    return (select(width == 0, 1, width - 1) / workgroup_size) + 1;
}

MeshletReference
get_meshlet_reference(uint32_t global_meshlet_index, uint32_t cascade_index) {
    PrefixSumValue result = prefix_sum_binary_search(
        prefix_sum_buffer_for_cascade(cascade_index),
        global_meshlet_index
    );
    Instance instance =
        InstanceBuffer(get_uniforms().instances).instances[result.index];
    MeshInfo mesh_info = MeshInfoBuffer(instance.mesh_info_address).mesh_info;
    uint8_t lod_level_index = LodLevelBuffer(get_uniforms().lod_levels).lod_levels[result.index];

    LodLevel lod_level = mesh_info.lods[lod_level_index];

    uint32_t meshlets_start = (result.sum - lod_level.num_meshlets);

    uint32_t local_meshlet_index =
        global_meshlet_index - meshlets_start + lod_level.meshlets_offset;

    MeshletReference reference;
    reference.instance_index = result.index;
    reference.meshlet_index = uint16_t(local_meshlet_index);
    return reference;
}

void reset_counters() {
    DrawCallBuffer draw_call_buf = DrawCallBuffer(get_uniforms().draw_calls);
    draw_call_buf.num_opaque = 0;
    draw_call_buf.num_alpha_clip = 0;

    prefix_sum_reset(prefix_sum_buffer_for_cascade(0));
    prefix_sum_reset(prefix_sum_buffer_for_cascade(1));
    prefix_sum_reset(prefix_sum_buffer_for_cascade(2));
    prefix_sum_reset(prefix_sum_buffer_for_cascade(3));
}
