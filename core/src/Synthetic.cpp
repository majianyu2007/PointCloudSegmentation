#include "pcseg/Synthetic.h"

#include <random>
#include <cmath>

namespace pcseg {

LabeledCloud makeTwoPlanes(int perPlane, float noise) {
    LabeledCloud lc;
    lc.numClasses = 2;
    std::mt19937 rng(123);
    std::normal_distribution<float> gauss(0.0f, noise);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);

    // 平面 A：z = 0，法向 +z，铺在 x,y in [0,1]
    for (int i = 0; i < perPlane; ++i) {
        float x = uni(rng), y = uni(rng);
        lc.cloud.addPoint(Vec3(x + gauss(rng), y + gauss(rng), 0.0f + gauss(rng)));
        lc.labels.push_back(0);
    }
    // 平面 B：x = 2，法向 +x，铺在 y,z in [0,1]
    // 故意和平面 A 拉开一段距离（不相接）：这样既测"朝向不同要分开"，
    // 也测"空间不连通要分开"。若两面在棱边相接，区域生长会沿棱边法向渐变
    // 处发生泄漏而把两面并到一起——这是区域生长的已知局限，在报告中单独讨论。
    for (int i = 0; i < perPlane; ++i) {
        float y = uni(rng), z = uni(rng);
        lc.cloud.addPoint(Vec3(2.0f + gauss(rng), y + gauss(rng), z + gauss(rng)));
        lc.labels.push_back(1);
    }
    return lc;
}

LabeledCloud makeScene(float noise, unsigned int seed) {
    LabeledCloud lc;
    lc.numClasses = 4;
    std::mt19937 rng(seed);
    std::normal_distribution<float> gauss(0.0f, noise);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);
    const float PI = 3.14159265358979323846f;

    auto add = [&](const Vec3& p, int label) {
        lc.cloud.addPoint(Vec3(p.x + gauss(rng), p.y + gauss(rng), p.z + gauss(rng)));
        lc.labels.push_back(label);
    };

    // 0) 地面：z = 0 的大平面，范围 [-2,2] x [-2,2]
    for (int i = 0; i < 3000; ++i) {
        float x = uni(rng) * 4.0f - 2.0f;
        float y = uni(rng) * 4.0f - 2.0f;
        add(Vec3(x, y, 0.0f), 0);
    }

    // 1) 球：球心 (-0.7, -0.5, 0.6)，半径 0.6（均匀采样球面）
    {
        Vec3 c(-0.7f, -0.5f, 0.6f);
        float radius = 0.6f;
        for (int i = 0; i < 2200; ++i) {
            // 球面均匀采样
            float u = uni(rng), v = uni(rng);
            float theta = 2.0f * PI * u;
            float phi = std::acos(2.0f * v - 1.0f);
            Vec3 dir(std::sin(phi) * std::cos(theta),
                     std::sin(phi) * std::sin(theta),
                     std::cos(phi));
            add(c + dir * radius, 1);
        }
    }

    // 2) 圆柱：竖直放置，轴心 (0.9, 0.7)，半径 0.4，高 0~1.2（只采样侧面）
    {
        float cx = 0.9f, cy = 0.7f, radius = 0.4f, height = 1.2f;
        for (int i = 0; i < 2200; ++i) {
            float theta = 2.0f * PI * uni(rng);
            float z = uni(rng) * height;
            add(Vec3(cx + radius * std::cos(theta),
                     cy + radius * std::sin(theta), z), 2);
        }
    }

    // 3) 斜面：从地面斜插上去的一块平板（法向量明显不同于地面）
    {
        // 参数化：沿 u（宽）和 v（高）两个方向铺点
        Vec3 origin(-1.6f, 1.2f, 0.0f);
        Vec3 du(1.4f, 0.0f, 0.0f);    // 宽度方向
        Vec3 dv(0.0f, 0.5f, 0.9f);    // 高度方向（带倾斜）
        for (int i = 0; i < 1600; ++i) {
            float u = uni(rng), v = uni(rng);
            add(origin + du * u + dv * v, 3);
        }
    }

    return lc;
}

} // namespace pcseg
