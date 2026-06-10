#ifndef PCSEG_EIGEN3X3_H
#define PCSEG_EIGEN3X3_H

#include <cmath>
#include "pcseg/Vec3.h"

namespace pcseg {

// 一个 3x3 对称矩阵的特征值分解（Jacobi 旋转法）。
// 法向量估计需要对每个点的邻域协方差矩阵（3x3 对称）求特征值/特征向量：
//   最小特征值对应的特征向量就是法向量；
//   曲率 = 最小特征值 / 三个特征值之和。
// 这里不依赖 Eigen 等第三方库，自己实现一个经典的 Jacobi 迭代，规模很小（3x3）足够快。
struct SymEigen {
    float values[3];     // 特征值，已按从小到大排序
    Vec3  vectors[3];    // 对应的特征向量（与 values 一一对应，单位向量）
};

// 输入对称矩阵的 6 个独立元素：
//   | a00 a01 a02 |
//   | a01 a11 a12 |
//   | a02 a12 a22 |
inline SymEigen symmetricEigen(float a00, float a01, float a02,
                               float a11, float a12, float a22) {
    // A：工作用矩阵，迭代中会被逐步对角化
    double A[3][3] = {
        {a00, a01, a02},
        {a01, a11, a12},
        {a02, a12, a22}
    };
    // V：累积的旋转矩阵，最终每一列就是一个特征向量
    double V[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };

    // 最多迭代若干轮，每轮把当前最大的非对角元素旋转消掉
    for (int iter = 0; iter < 50; ++iter) {
        // 找出绝对值最大的非对角元素 A[p][q]
        int p = 0, q = 1;
        double maxOff = std::fabs(A[0][1]);
        if (std::fabs(A[0][2]) > maxOff) { maxOff = std::fabs(A[0][2]); p = 0; q = 2; }
        if (std::fabs(A[1][2]) > maxOff) { maxOff = std::fabs(A[1][2]); p = 1; q = 2; }

        // 非对角元素已经足够小，认为收敛
        if (maxOff < 1e-12) break;

        // 计算旋转角，使 A[p][q] 变 0
        double app = A[p][p];
        double aqq = A[q][q];
        double apq = A[p][q];
        double phi = 0.5 * std::atan2(2.0 * apq, aqq - app);
        double c = std::cos(phi);
        double s = std::sin(phi);

        // 用旋转矩阵 J 更新 A = J^T A J
        double Apk, Aqk;
        for (int k = 0; k < 3; ++k) {
            Apk = A[p][k];
            Aqk = A[q][k];
            A[p][k] = c * Apk - s * Aqk;
            A[q][k] = s * Apk + c * Aqk;
        }
        for (int k = 0; k < 3; ++k) {
            Apk = A[k][p];
            Aqk = A[k][q];
            A[k][p] = c * Apk - s * Aqk;
            A[k][q] = s * Apk + c * Aqk;
        }
        // 同步更新特征向量矩阵 V
        for (int k = 0; k < 3; ++k) {
            double Vkp = V[k][p];
            double Vkq = V[k][q];
            V[k][p] = c * Vkp - s * Vkq;
            V[k][q] = s * Vkp + c * Vkq;
        }
    }

    // 取对角线作为特征值，V 的每一列作为特征向量
    float eval[3] = { (float)A[0][0], (float)A[1][1], (float)A[2][2] };
    Vec3 evec[3] = {
        Vec3((float)V[0][0], (float)V[1][0], (float)V[2][0]),
        Vec3((float)V[0][1], (float)V[1][1], (float)V[2][1]),
        Vec3((float)V[0][2], (float)V[1][2], (float)V[2][2])
    };

    // 按特征值从小到大排序（简单的冒泡，只有 3 个元素）
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2 - i; ++j) {
            if (eval[j] > eval[j + 1]) {
                std::swap(eval[j], eval[j + 1]);
                std::swap(evec[j], evec[j + 1]);
            }
        }
    }

    SymEigen result;
    for (int i = 0; i < 3; ++i) {
        result.values[i] = eval[i];
        result.vectors[i] = evec[i].normalized();
    }
    return result;
}

} // namespace pcseg

#endif // PCSEG_EIGEN3X3_H
