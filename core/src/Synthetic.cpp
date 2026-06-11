#include "pcseg/Synthetic.h"
#include "pcseg/Primitive.h"

#include <memory>
#include <random>

namespace pcseg {

// 两个朝向不同、且空间分离的平面。法向量差 90°、互不相接，
// 正确的分割应当干净地分成 2 段（主要给单元测试用）。
LabeledCloud makeTwoPlanes(int perPlane, float noise) {
    LabeledCloud lc;
    lc.numClasses = 2;
    std::mt19937 rng(123);
    std::normal_distribution<float> gauss(0.0f, noise);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);

    // 用多态的几何体来描述场景：两块平面，只是位置/朝向不同
    std::vector<std::unique_ptr<Primitive>> prims;
    // 平面 A：z = 0，铺在 x,y in [0,1]
    prims.push_back(std::make_unique<PlanePrimitive>(
        0, Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0)));
    // 平面 B：x = 2，铺在 y,z in [0,1]（与 A 拉开距离、朝向不同）
    prims.push_back(std::make_unique<PlanePrimitive>(
        1, Vec3(2, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1)));

    for (const auto& p : prims) {
        p->sample(perPlane, rng, gauss, uni, lc.cloud, lc.labels);
    }
    return lc;
}

// 一个更丰富的演示场景：地面 + 球 + 圆柱 + 斜面，共 4 个几何体。
// 通过基类指针多态地调用各几何体的 sample()，新增形状无需改动本函数主体。
LabeledCloud makeScene(float noise, unsigned int seed) {
    LabeledCloud lc;
    lc.numClasses = 4;
    std::mt19937 rng(seed);
    std::normal_distribution<float> gauss(0.0f, noise);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);

    // 各几何体（标签 0~3）及其采样点数
    std::vector<std::unique_ptr<Primitive>> prims;
    prims.push_back(std::make_unique<PlanePrimitive>(      // 0 地面
        0, Vec3(-2, -2, 0), Vec3(4, 0, 0), Vec3(0, 4, 0)));
    prims.push_back(std::make_unique<SpherePrimitive>(     // 1 球
        1, Vec3(-0.7f, -0.5f, 0.6f), 0.6f));
    prims.push_back(std::make_unique<CylinderPrimitive>(   // 2 圆柱
        2, 0.9f, 0.7f, 0.4f, 1.2f));
    prims.push_back(std::make_unique<PlanePrimitive>(      // 3 斜面
        3, Vec3(-1.6f, 1.2f, 0.0f), Vec3(1.4f, 0, 0), Vec3(0, 0.5f, 0.9f)));

    const int counts[] = {3000, 2200, 2200, 1600};
    for (std::size_t i = 0; i < prims.size(); ++i) {
        prims[i]->sample(counts[i], rng, gauss, uni, lc.cloud, lc.labels);
    }
    return lc;
}

} // namespace pcseg
