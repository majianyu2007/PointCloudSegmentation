#ifndef PCSEG_KDTREE_H
#define PCSEG_KDTREE_H

#include <vector>
#include "pcseg/Vec3.h"

namespace pcseg {

// 三维 KD 树，用来做 k 近邻查询。
// 法向量估计和区域生长都需要反复问"离某个点最近的 k 个点是谁"，
// 如果每次都遍历所有点会是 O(N^2)，点多了就很慢；KD 树把它降到平均 O(log N)。
class KdTree {
public:
    KdTree() = default;

    // 用一组点建树。会保存 points 的指针引用，调用期间 points 不要被销毁。
    void build(const std::vector<Vec3>& points);

    // 查询离 query 最近的 k 个点，返回它们在原始点数组里的下标
    // （结果按距离从近到远排序，第一个通常就是 query 自己，如果它在点集中）。
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

    // 递归建树：对 indices[first, last) 这段下标建子树，返回子树根在 nodes_ 里的下标
    int buildRecursive(std::vector<int>& indices, int first, int last, int depth);

    // 递归查询，维护一个"当前最近的 k 个"列表
    void searchRecursive(int nodeIdx, const Vec3& query, int k,
                         std::vector<std::pair<float, int>>& heap) const;
};

} // namespace pcseg

#endif // PCSEG_KDTREE_H
