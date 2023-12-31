#include "meshlets.h"

Meshlets build_meshlets(
    uint8_t* indices,
    size_t indices_count,
    float* positions,
    size_t vertices_count,
    bool uses_32_bit_indices
) {
    auto max_vertices = MAX_MESHLET_UNIQUE_VERTICES;
    auto max_triangles = MAX_MESHLET_TRIANGLES;
    // Given that I want to render a lot of foliage
    // which is double-sided and doesn't benefit from
    // cone culling, we can turn this down.
    auto cone_weight = 0.25f;

    size_t max_meshlets =
        meshopt_buildMeshletsBound(indices_count, max_vertices, max_triangles);
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    std::vector<unsigned int> meshlet_indices_32bit(
        max_meshlets * max_vertices
    );
    std::vector<unsigned char> micro_indices(max_meshlets * max_triangles * 3);

    size_t meshlet_count = 0;

    auto stride = sizeof(float) * 3;

    if (uses_32_bit_indices) {
        meshlet_count = meshopt_buildMeshlets(
            meshlets.data(),
            meshlet_indices_32bit.data(),
            micro_indices.data(),
            reinterpret_cast<uint32_t*>(indices),
            indices_count,
            positions,
            vertices_count,
            stride,
            max_vertices,
            max_triangles,
            cone_weight
        );
    } else {
        meshlet_count = meshopt_buildMeshlets(
            meshlets.data(),
            meshlet_indices_32bit.data(),
            micro_indices.data(),
            reinterpret_cast<uint16_t*>(indices),
            indices_count,
            positions,
            vertices_count,
            stride,
            max_vertices,
            max_triangles,
            cone_weight
        );
    }

    const meshopt_Meshlet& last = meshlets[meshlet_count - 1];

    meshlet_indices_32bit.resize(last.vertex_offset + last.vertex_count);
    micro_indices.resize(
        last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3)
    );
    meshlets.resize(meshlet_count);

    std::vector<meshopt_Bounds> meshlet_bounds(meshlet_count);

    for (size_t i = 0; i < meshlet_count; i++) {
        auto meshlet = meshlets[i];
        meshlet_bounds[i] = meshopt_computeMeshletBounds(
            &meshlet_indices_32bit[meshlet.vertex_offset],
            &micro_indices[meshlet.triangle_offset],
            meshlet.triangle_count,
            positions,
            vertices_count,
            stride
        );

        // This means that I've done something wrong.
        if (meshlet_bounds[i].radius == 0) {
            dbg(meshlet.triangle_count,
                meshlet.vertex_count,
                meshlet.triangle_offset);
        }
    }

    std::vector<Meshlet> final_meshlets(meshlet_count);

    for (size_t i = 0; i < meshlet_count; i++) {
        auto meshlet = meshlets[i];
        meshopt_Bounds bounds = meshlet_bounds[i];
        final_meshlets[i] = Meshlet {
            .cone_apex = glm::vec3(
                bounds.cone_apex[0],
                bounds.cone_apex[1],
                bounds.cone_apex[2]
            ),
            .cone_axis = glm::vec3(
                bounds.cone_axis[0],
                bounds.cone_axis[1],
                bounds.cone_axis[2]
            ),
            .cone_cutoff = bounds.cone_cutoff,
            .bounding_sphere = glm::vec4(
                bounds.center[0],
                bounds.center[1],
                bounds.center[2],
                bounds.radius
            ),
            .triangle_offset = meshlet.triangle_offset,
            .index_offset = meshlet.vertex_offset,
            .triangle_count = static_cast<uint8_t>(meshlet.triangle_count),
            .index_count = static_cast<uint8_t>(meshlet.vertex_count)};
    }

    if (uses_32_bit_indices) {
        return {
            .meshlets = final_meshlets,
            .micro_indices = micro_indices,
            .indices_32bit = meshlet_indices_32bit,
            .indices_16bit = {}};
    } else {
        // No need to use 32-bit indices.
        std::vector<uint16_t> meshlet_indices_16bit(meshlet_indices_32bit.size()
        );

        for (size_t i = 0; i < meshlet_indices_32bit.size(); i++) {
            auto index = meshlet_indices_32bit[i];
            assert(index < (1 << 16));
            meshlet_indices_16bit[i] = index;
        }

        return {
            .meshlets = final_meshlets,
            .micro_indices = micro_indices,
            .indices_32bit = {},
            .indices_16bit = meshlet_indices_16bit};
    }
}
