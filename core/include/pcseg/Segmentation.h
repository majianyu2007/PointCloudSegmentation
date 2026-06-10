#ifndef PCSEG_SEGMENTATION_H
#define PCSEG_SEGMENTATION_H

#include <vector>
#include "pcseg/PointCloud.h"
#include "pcseg/KdTree.h"

namespace pcseg {

// 分割参数。
struct SegParams {
    int k = 16;                        // 构建近邻图时每个点的邻居数
    int minClusterSize = 30;           // 小于这个点数的段视为噪声，丢弃（标 -1）
    float curvatureThreshold = 0.08f;  // 种子曲率阈值：只有比它平坦的点才继续向外扩张
    int levels = 3;                    // 由粗到细的层数
    float coarseSmoothnessDeg = 30.0f; // 最粗一层的法向夹角阈值（度）——越大越容易合并
    float fineSmoothnessDeg = 10.0f;   // 最细一层的法向夹角阈值（度）——越小分得越细
};

// 分割结果：保存由粗到细每一层的标签，供界面做"迭代过程回放"。
struct SegResult {
    // levelLabels[L][i]：第 L 层、第 i 个点所属的段号（0,1,2,...），-1 表示噪声/未分配。
    std::vector<std::vector<int>> levelLabels;
    std::vector<int> segmentCount;  // 每一层的段数
    int levels() const { return (int)levelLabels.size(); }
};

// 为整个点云构建 k 近邻邻接表：adjacency[i] = 第 i 个点的 k 个近邻的下标。
// 区域生长就是在这张图上做"连通扩张"。
std::vector<std::vector<int>> buildKnnAdjacency(const PointCloud& cloud,
                                                const KdTree& tree, int k);

// 在 allowed[i]==1 的点子集内做一次区域生长。
// 返回每个点的段标签（从 0 开始；-1 表示不在子集内、或所在段太小被丢弃）。
// segCountOut 返回有效段的数量。
std::vector<int> regionGrow(const PointCloud& cloud,
                            const std::vector<std::vector<int>>& adjacency,
                            const std::vector<char>& allowed,
                            float smoothnessRad,
                            float curvatureThreshold,
                            int minClusterSize,
                            int& segCountOut);

// 完整的"由粗到细"分割：
//   第 0 层用最大的夹角阈值，得到少数几个大块；
//   之后每一层在上一层每个段的内部，用更小的夹角阈值继续细分；
//   阈值从 coarse 线性过渡到 fine。
// cloud 需要已经有法向量和曲率（先调用 estimateNormals）。
SegResult segment(const PointCloud& cloud, const KdTree& tree, const SegParams& params);

} // namespace pcseg

#endif // PCSEG_SEGMENTATION_H
