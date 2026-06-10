#include "pcseg/PointCloud.h"

#include <limits>

namespace pcseg {

void PointCloud::boundingBox(Vec3& mn, Vec3& mx) const {
    if (points.empty()) {
        mn = Vec3(0, 0, 0);
        mx = Vec3(0, 0, 0);
        return;
    }
    float big = std::numeric_limits<float>::max();
    mn = Vec3(big, big, big);
    mx = Vec3(-big, -big, -big);
    for (const Vec3& p : points) {
        if (p.x < mn.x) mn.x = p.x;
        if (p.y < mn.y) mn.y = p.y;
        if (p.z < mn.z) mn.z = p.z;
        if (p.x > mx.x) mx.x = p.x;
        if (p.y > mx.y) mx.y = p.y;
        if (p.z > mx.z) mx.z = p.z;
    }
}

Vec3 PointCloud::centroid() const {
    if (points.empty()) return Vec3(0, 0, 0);
    Vec3 sum(0, 0, 0);
    for (const Vec3& p : points) sum += p;
    return sum / static_cast<float>(points.size());
}

} // namespace pcseg
