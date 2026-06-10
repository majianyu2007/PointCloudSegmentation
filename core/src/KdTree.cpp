#include "pcseg/KdTree.h"

#include <algorithm>
#include <cmath>

namespace pcseg {

void KdTree::build(const std::vector<Vec3>& points) {
    points_ = &points;
    nodes_.clear();
    root_ = -1;
    if (points.empty()) return;

    nodes_.reserve(points.size());
    // 准备一个下标数组，建树时只对下标做划分，不真正移动点数据
    std::vector<int> indices(points.size());
    for (int i = 0; i < (int)points.size(); ++i) indices[i] = i;

    root_ = buildRecursive(indices, 0, (int)indices.size(), 0);
}

// 取出坐标的第 axis 个分量
static inline float axisValue(const Vec3& p, int axis) {
    if (axis == 0) return p.x;
    if (axis == 1) return p.y;
    return p.z;
}

int KdTree::buildRecursive(std::vector<int>& indices, int first, int last, int depth) {
    if (first >= last) return -1;

    int axis = depth % 3;        // 每往下一层换一个分割轴：x -> y -> z -> x ...
    int mid = (first + last) / 2;

    // 用 nth_element 把中位数那个点放到 mid 位置（按当前轴的坐标排序），
    // 左边都比它小、右边都比它大。这样建出来的树比较平衡。
    const std::vector<Vec3>& pts = *points_;
    std::nth_element(indices.begin() + first, indices.begin() + mid, indices.begin() + last,
                     [&](int a, int b) {
                         return axisValue(pts[a], axis) < axisValue(pts[b], axis);
                     });

    Node node;
    node.pointIndex = indices[mid];
    node.axis = axis;

    int nodeIdx = (int)nodes_.size();
    nodes_.push_back(node);   // 先占位，子树下标随后回填

    int leftChild = buildRecursive(indices, first, mid, depth + 1);
    int rightChild = buildRecursive(indices, mid + 1, last, depth + 1);
    nodes_[nodeIdx].left = leftChild;
    nodes_[nodeIdx].right = rightChild;
    return nodeIdx;
}

std::vector<int> KdTree::kNearest(const Vec3& query, int k) const {
    std::vector<int> result;
    if (root_ < 0 || k <= 0) return result;

    // 用一个"最大堆"维护当前找到的最近 k 个点：堆顶是这 k 个里最远的那个，
    // 这样遇到更近的点时直接替换堆顶即可。元素是 (距离平方, 点下标)。
    std::vector<std::pair<float, int>> heap;
    heap.reserve(k + 1);
    searchRecursive(root_, query, k, heap);

    // 堆里就是结果，按距离从近到远排序后返回下标
    std::sort(heap.begin(), heap.end());
    result.reserve(heap.size());
    for (const auto& item : heap) result.push_back(item.second);
    return result;
}

void KdTree::searchRecursive(int nodeIdx, const Vec3& query, int k,
                             std::vector<std::pair<float, int>>& heap) const {
    if (nodeIdx < 0) return;

    const Node& node = nodes_[nodeIdx];
    const Vec3& p = (*points_)[node.pointIndex];

    // 当前点到查询点的距离，先尝试放进 k 近邻堆
    float d2 = distanceSquared(query, p);
    if ((int)heap.size() < k) {
        heap.push_back({d2, node.pointIndex});
        std::push_heap(heap.begin(), heap.end());   // 默认大顶堆，堆顶是最大距离
    } else if (d2 < heap.front().first) {
        std::pop_heap(heap.begin(), heap.end());
        heap.back() = {d2, node.pointIndex};
        std::push_heap(heap.begin(), heap.end());
    }

    // 决定先搜哪一侧：查询点在分割面的哪一边就先搜哪边
    float diff = axisValue(query, node.axis) - axisValue(p, node.axis);
    int nearSide = (diff < 0) ? node.left : node.right;
    int farSide = (diff < 0) ? node.right : node.left;

    searchRecursive(nearSide, query, k, heap);

    // 另一侧要不要搜？只有当"分割面距离"比当前第 k 近还要近时，
    // 另一侧才可能有更近的点，否则可以剪枝跳过。
    bool needFar = (int)heap.size() < k || (diff * diff) < heap.front().first;
    if (needFar) {
        searchRecursive(farSide, query, k, heap);
    }
}

} // namespace pcseg
