#include "pcseg/Segmentation.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace pcseg {

std::vector<std::vector<int>> buildKnnAdjacency(const PointCloud& cloud,
                                                const KdTree& tree, int k) {
    const int n = (int)cloud.points.size();
    std::vector<std::vector<int>> adjacency(n);
    for (int i = 0; i < n; ++i) {
        // kNearest 通常会把点自己作为第一个返回，扩张时无所谓，保留即可
        adjacency[i] = tree.kNearest(cloud.points[i], k);
    }
    return adjacency;
}

std::vector<int> regionGrow(const PointCloud& cloud,
                            const std::vector<std::vector<int>>& adjacency,
                            const std::vector<char>& allowed,
                            float smoothnessRad,
                            float curvatureThreshold,
                            int minClusterSize,
                            int& segCountOut) {
    const int n = (int)cloud.points.size();
    std::vector<int> labels(n, -1);
    const float cosThreshold = std::cos(smoothnessRad);  // 用余弦比较，省去每次 acos

    // 把子集内的点按曲率从小到大排序，越平坦的越先当种子（区域生长的经典做法）
    std::vector<int> order;
    order.reserve(n);
    for (int i = 0; i < n; ++i) {
        if (allowed[i]) order.push_back(i);
    }
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return cloud.curvature[a] < cloud.curvature[b];
    });

    int segId = 0;
    for (int seed : order) {
        if (labels[seed] != -1) continue;  // 已经归到某个段了

        // 从这个种子开始，广度优先地向外扩张出一个段
        std::queue<int> q;
        q.push(seed);
        labels[seed] = segId;

        while (!q.empty()) {
            int cur = q.front();
            q.pop();
            const Vec3& nCur = cloud.normals[cur];

            for (int nb : adjacency[cur]) {
                if (!allowed[nb] || labels[nb] != -1) continue;

                // 当前点与邻居法向量的夹角，用 |cos| 比较（忽略法向正反方向）
                float c = std::fabs(dot(nCur, cloud.normals[nb]));
                if (c < cosThreshold) continue;   // 夹角太大，不属于同一光滑面

                labels[nb] = segId;               // 并入当前段
                // 只有足够平坦的点才继续作为新的扩张前沿，避免越过棱边
                if (cloud.curvature[nb] < curvatureThreshold) {
                    q.push(nb);
                }
            }
        }
        ++segId;
    }

    // 统计每个段的点数，丢弃太小的段（标回 -1），再把段号压缩成连续的 0..K-1
    std::vector<int> count(segId, 0);
    for (int i = 0; i < n; ++i) {
        if (labels[i] >= 0) count[labels[i]]++;
    }
    std::vector<int> remap(segId, -1);
    int kept = 0;
    for (int s = 0; s < segId; ++s) {
        if (count[s] >= minClusterSize) remap[s] = kept++;
    }
    for (int i = 0; i < n; ++i) {
        if (labels[i] >= 0) labels[i] = remap[labels[i]];
    }

    segCountOut = kept;
    return labels;
}

SegResult segment(const PointCloud& cloud, const KdTree& tree, const SegParams& params) {
    SegResult result;
    const int n = (int)cloud.points.size();
    if (n == 0) return result;

    const double deg2rad = 3.14159265358979323846 / 180.0;
    std::vector<std::vector<int>> adjacency = buildKnnAdjacency(cloud, tree, params.k);

    int levels = std::max(1, params.levels);

    // 第 0 层：整片点云都参与，用最粗（最大）的夹角阈值
    std::vector<char> allowAll(n, 1);
    int count0 = 0;
    float angle0 = (float)(params.coarseSmoothnessDeg * deg2rad);
    std::vector<int> labels0 = regionGrow(cloud, adjacency, allowAll, angle0,
                                          params.curvatureThreshold,
                                          params.minClusterSize, count0);
    result.levelLabels.push_back(labels0);
    result.segmentCount.push_back(count0);

    // 之后每一层：在上一层每个段的内部，用更小的夹角阈值继续细分
    for (int L = 1; L < levels; ++L) {
        // 夹角阈值从 coarse 线性过渡到 fine
        float t = (float)L / (float)(levels - 1);
        float deg = params.coarseSmoothnessDeg +
                    t * (params.fineSmoothnessDeg - params.coarseSmoothnessDeg);
        float angle = (float)(deg * deg2rad);

        const std::vector<int>& prev = result.levelLabels.back();
        int prevCount = result.segmentCount.back();

        std::vector<int> labels(n, -1);
        int globalSeg = 0;
        // 对上一层的每一个段，单独再做一次区域生长
        for (int p = 0; p < prevCount; ++p) {
            std::vector<char> allowed(n, 0);
            for (int i = 0; i < n; ++i) {
                if (prev[i] == p) allowed[i] = 1;
            }
            int subCount = 0;
            std::vector<int> sub = regionGrow(cloud, adjacency, allowed, angle,
                                              params.curvatureThreshold,
                                              params.minClusterSize, subCount);
            // 把这个父段细分出来的局部段号，平移到全局段号上
            for (int i = 0; i < n; ++i) {
                if (sub[i] >= 0) labels[i] = globalSeg + sub[i];
            }
            globalSeg += subCount;
        }
        result.levelLabels.push_back(labels);
        result.segmentCount.push_back(globalSeg);
    }

    return result;
}

} // namespace pcseg
