// Adapted from meshoptimizer.

static void
computeBoundingSphere(float result[4], const float* points, size_t count) {
    assert(count > 0);

    // find extremum points along all 3 axes; for each axis we get a pair of points with min/max coordinates
    size_t pmin[3] = {0, 0, 0};
    size_t pmax[3] = {0, 0, 0};

    for (size_t i = 0; i < count; ++i) {
        const float* p = &points[i * 3];

        for (int axis = 0; axis < 3; ++axis) {
            pmin[axis] =
                (p[axis] < points[3 * pmin[axis] + axis]) ? i : pmin[axis];
            pmax[axis] =
                (p[axis] > points[3 * pmax[axis] + axis]) ? i : pmax[axis];
        }
    }

    // find the pair of points with largest distance
    float paxisd2 = 0;
    int paxis = 0;

    for (int axis = 0; axis < 3; ++axis) {
        const float* p1 = &points[3 * pmin[axis]];
        const float* p2 = &points[3 * pmax[axis]];

        float d2 = (p2[0] - p1[0]) * (p2[0] - p1[0])
            + (p2[1] - p1[1]) * (p2[1] - p1[1])
            + (p2[2] - p1[2]) * (p2[2] - p1[2]);

        if (d2 > paxisd2) {
            paxisd2 = d2;
            paxis = axis;
        }
    }

    // use the longest segment as the initial sphere diameter
    const float* p1 = &points[3 * pmin[paxis]];
    const float* p2 = &points[3 * pmax[paxis]];

    float center[3] = {
        (p1[0] + p2[0]) / 2,
        (p1[1] + p2[1]) / 2,
        (p1[2] + p2[2]) / 2};
    float radius = sqrtf(paxisd2) / 2;

    // iteratively adjust the sphere up until all points fit
    for (size_t i = 0; i < count; ++i) {
        const float* p = &points[i * 3];
        float d2 = (p[0] - center[0]) * (p[0] - center[0])
            + (p[1] - center[1]) * (p[1] - center[1])
            + (p[2] - center[2]) * (p[2] - center[2]);

        if (d2 > radius * radius) {
            float d = sqrtf(d2);
            assert(d > 0);

            float k = 0.5f + (radius / d) / 2;

            center[0] = center[0] * k + p[0] * (1 - k);
            center[1] = center[1] * k + p[1] * (1 - k);
            center[2] = center[2] * k + p[2] * (1 - k);
            radius = (radius + d) / 2;
        }
    }

    result[0] = center[0];
    result[1] = center[1];
    result[2] = center[2];
    result[3] = radius;
}
