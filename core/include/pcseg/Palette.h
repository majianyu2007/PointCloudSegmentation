#ifndef PCSEG_PALETTE_H
#define PCSEG_PALETTE_H

#include <cmath>
#include "pcseg/Vec3.h"

namespace pcseg {

// 给分割出来的每个段分配一种好区分的颜色。
// 用"黄金角"在色相环上跳着取色（相邻段号的颜色差异大），饱和度和亮度固定。
// 返回的颜色分量在 0~1 之间（OpenGL 用）。label < 0（噪声）返回灰色。
inline Vec3 colorForLabel(int label) {
    if (label < 0) return Vec3(0.5f, 0.5f, 0.5f);

    // 黄金角约 137.5°，每个段号转一下色相，铺得比较均匀
    float hue = std::fmod(label * 137.508f, 360.0f) / 360.0f;  // 归一化到 0~1
    float s = 0.65f;
    float v = 0.95f;

    // HSV 转 RGB
    float h = hue * 6.0f;
    int i = (int)std::floor(h) % 6;
    float f = h - std::floor(h);
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    switch (i) {
        case 0: return Vec3(v, t, p);
        case 1: return Vec3(q, v, p);
        case 2: return Vec3(p, v, t);
        case 3: return Vec3(p, q, v);
        case 4: return Vec3(t, p, v);
        default: return Vec3(v, p, q);
    }
}

} // namespace pcseg

#endif // PCSEG_PALETTE_H
