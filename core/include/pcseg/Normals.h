#ifndef PCSEG_NORMALS_H
#define PCSEG_NORMALS_H

#include "pcseg/PointCloud.h"
#include "pcseg/KdTree.h"

namespace pcseg {

// 法向量与曲率估计。
// 做法（主成分分析 PCA）：对每个点，取它的 k 个近邻，求这些邻域点的协方差矩阵，
// 做特征值分解。三个特征值 λ0 <= λ1 <= λ2：
//   - 最小特征值 λ0 对应的特征向量 = 该点的法向量（邻域最"扁"的方向）；
//   - 曲率 = λ0 / (λ0 + λ1 + λ2)，越小说明邻域越平坦。
//
// 估计完成后结果直接写回 cloud.normals 和 cloud.curvature。
// tree 必须是已经用 cloud.points 建好的 KD 树。
void estimateNormals(PointCloud& cloud, const KdTree& tree, int k = 16);

} // namespace pcseg

#endif // PCSEG_NORMALS_H
