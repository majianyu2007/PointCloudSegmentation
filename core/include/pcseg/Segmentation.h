#ifndef PCSEG_SEGMENTATION_H
#define PCSEG_SEGMENTATION_H

#include <vector>
#include "pcseg/PointCloud.h"
#include "pcseg/KdTree.h"

namespace pcseg {

struct SegParams {
    int k = 16;                        // 构建近邻图时每个点的邻居数
    int minClusterSize = 30;           // 小于这个点数的段视为噪声，丢弃（标 -1）
    float curvatureThreshold = 0.08f;  // 种子曲率阈值：只有比它平坦的点才继续向外扩张
    int levels = 3;                    // 由粗到细的层数
    float coarseSmoothnessDeg = 30.0f; // 最粗一层的法向夹角阈值（度）——越大越容易合并
    float fineSmoothnessDeg = 10.0f;   // 最细一层的法向夹角阈值（度）——越小分得越细
};

struct SegResult {
    // levelLabels[L][i]：第 L 层、第 i 个点所属的段号（0,1,2,...），-1 表示噪声/未分配。
    std::vector<std::vector<int>> levelLabels;
    std::vector<int> segmentCount;  // 每一层的段数
    int levels() const { return (int)levelLabels.size(); }
};

std::vector<std::vector<int>> buildKnnAdjacency(const PointCloud& cloud,
                                                const KdTree& tree, int k);

std::vector<int> regionGrow(const PointCloud& cloud,
                            const std::vector<std::vector<int>>& adjacency,
                            const std::vector<char>& allowed,
                            float smoothnessRad,
                            float curvatureThreshold,
                            int minClusterSize,
                            int& segCountOut);

SegResult segment(const PointCloud& cloud, const KdTree& tree, const SegParams& params);

} // namespace pcseg

#endif // PCSEG_SEGMENTATION_H
