#include "pcseg/Normals.h"
#include "pcseg/Eigen3x3.h"

namespace pcseg {

void estimateNormals(PointCloud& cloud, const KdTree& tree, int k) {
    const std::size_t n = cloud.points.size();
    cloud.normals.assign(n, Vec3(0, 0, 0));
    cloud.curvature.assign(n, 0.0f);
    if (n == 0) return;

    if (k < 3) k = 3;

    for (std::size_t i = 0; i < n; ++i) {
        std::vector<int> nb = tree.kNearest(cloud.points[i], k);
        if (nb.size() < 3) {
            cloud.normals[i] = Vec3(0, 0, 1);
            cloud.curvature[i] = 0.0f;
            continue;
        }

        Vec3 mean(0, 0, 0);
        for (int idx : nb) mean += cloud.points[idx];
        mean = mean / static_cast<float>(nb.size());

        // 邻域协方差矩阵是对称矩阵，只保存 6 个独立元素。
        float c00 = 0, c01 = 0, c02 = 0, c11 = 0, c12 = 0, c22 = 0;
        for (int idx : nb) {
            Vec3 d = cloud.points[idx] - mean;
            c00 += d.x * d.x;
            c01 += d.x * d.y;
            c02 += d.x * d.z;
            c11 += d.y * d.y;
            c12 += d.y * d.z;
            c22 += d.z * d.z;
        }
        float inv = 1.0f / static_cast<float>(nb.size());
        c00 *= inv; c01 *= inv; c02 *= inv; c11 *= inv; c12 *= inv; c22 *= inv;

        SymEigen e = symmetricEigen(c00, c01, c02, c11, c12, c22);
        Vec3 normal = e.vectors[0];

        float sum = e.values[0] + e.values[1] + e.values[2];
        float curv = (sum > 1e-12f) ? (e.values[0] / sum) : 0.0f;

        // 统一法向朝向便于显示；分割时使用法向夹角的绝对值。
        if (normal.z < 0) normal = normal * -1.0f;

        cloud.normals[i] = normal.normalized();
        cloud.curvature[i] = curv;
    }
}

} // namespace pcseg
