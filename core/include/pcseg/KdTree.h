#ifndef PCSEG_KDTREE_H
#define PCSEG_KDTREE_H

#include <vector>
#include "pcseg/Vec3.h"

namespace pcseg {

// 三维 KD 树，用于近邻查询。
class KdTree {
public:
    KdTree() = default;

    // 保存 points 的引用，调用期间 points 不能被销毁。
    void build(const std::vector<Vec3>& points);

    // 返回最近的 k 个点在原始点数组中的下标。
    std::vector<int> kNearest(const Vec3& query, int k) const;

    bool empty() const { return nodes_.empty(); }

private:
    struct Node {
        int pointIndex;   // 该节点对应的点在原数组里的下标
        int axis;         // 分割轴：0=x, 1=y, 2=z
        int left = -1;    // 左子树节点下标（-1 表示空）
        int right = -1;   // 右子树节点下标
    };

    const std::vector<Vec3>* points_ = nullptr;
    std::vector<Node> nodes_;
    int root_ = -1;

    int buildRecursive(std::vector<int>& indices, int first, int last, int depth);

    void searchRecursive(int nodeIdx, const Vec3& query, int k,
                         std::vector<std::pair<float, int>>& heap) const;
};

} // namespace pcseg

#endif // PCSEG_KDTREE_H
