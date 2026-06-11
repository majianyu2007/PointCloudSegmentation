#include "pcseg/Normals.h"
#include "pcseg/Eigen3x3.h"

namespace pcseg {

void estimateNormals(PointCloud& cloud, const KdTree& tree, int k) {
    const std::size_t n = cloud.points.size();
    cloud.normals.assign(n, Vec3(0, 0, 0));
    cloud.curvature.assign(n, 0.0f);
    if (n == 0) return;

    // 邻域太大没意义，至少要 3 个点才能定一个面
    if (k < 3) k = 3;

    for (std::size_t i = 0; i < n; ++i) {
        std::vector<int> nb = tree.kNearest(cloud.points[i], k);
        if (nb.size() < 3) {
            // 邻居太少，给个默认向上的法向量
            cloud.normals[i] = Vec3(0, 0, 1);
            cloud.curvature[i] = 0.0f;
            continue;
        }

        // 1) 邻域质心
        Vec3 mean(0, 0, 0);
        for (int idx : nb) mean += cloud.points[idx];
        mean = mean / static_cast<float>(nb.size());

        // 2) 协方差矩阵（对称，只需算 6 个元素）
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

        // 3) 特征值分解：最小特征值的特征向量就是法向量
        SymEigen e = symmetricEigen(c00, c01, c02, c11, c12, c22);
        Vec3 normal = e.vectors[0];

        // 4) 曲率 = 最小特征值 / 三个特征值之和
        float sum = e.values[0] + e.values[1] + e.values[2];
        float curv = (sum > 1e-12f) ? (e.values[0] / sum) : 0.0f;

        // 让法向量大致朝向 +Z（统一一下方向，纯粹是为了显示好看；
        // 区域生长里比较法向时用的是夹角的绝对值，不依赖这个符号）。
        if (normal.z < 0) normal = normal * -1.0f;

        cloud.normals[i] = normal.normalized();
        cloud.curvature[i] = curv;
    }
}

} // namespace pcseg
