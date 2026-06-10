#ifndef PCSEG_POINTCLOUD_H
#define PCSEG_POINTCLOUD_H

#include <vector>
#include "pcseg/Vec3.h"

namespace pcseg {

// 点云数据结构。
// 这是整个系统最核心的数据：一堆三维点，外加（计算之后才有的）法向量和曲率。
// points 一定有值；normals / curvature 在调用法向量估计之后才被填充。
class PointCloud {
public:
    std::vector<Vec3> points;      // 每个点的坐标
    std::vector<Vec3> normals;     // 每个点的法向量（估计之后才有，大小同 points）
    std::vector<float> curvature;  // 每个点的表面曲率（估计之后才有）

    std::size_t size() const { return points.size(); }
    bool empty() const { return points.empty(); }

    void clear() {
        points.clear();
        normals.clear();
        curvature.clear();
    }

    void addPoint(const Vec3& p) { points.push_back(p); }

    // 是否已经算过法向量
    bool hasNormals() const { return normals.size() == points.size() && !points.empty(); }

    // 包围盒。min / max 通过引用返回。空点云时返回两个零向量。
    void boundingBox(Vec3& mn, Vec3& mx) const;

    // 所有点的质心（重心）
    Vec3 centroid() const;
};

} // namespace pcseg

#endif // PCSEG_POINTCLOUD_H
