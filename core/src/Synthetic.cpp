#include "pcseg/Synthetic.h"
#include "pcseg/Primitive.h"

#include <memory>
#include <random>

namespace pcseg {

LabeledCloud makeTwoPlanes(int perPlane, float noise) {
    LabeledCloud lc;
    lc.numClasses = 2;
    std::mt19937 rng(123);
    std::normal_distribution<float> gauss(0.0f, noise);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);

    std::vector<std::unique_ptr<Primitive>> prims;
    prims.push_back(std::make_unique<PlanePrimitive>(
        0, Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0)));
    prims.push_back(std::make_unique<PlanePrimitive>(
        1, Vec3(2, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1)));

    for (const auto& p : prims) {
        p->sample(perPlane, rng, gauss, uni, lc.cloud, lc.labels);
    }
    return lc;
}

LabeledCloud makeScene(float noise, unsigned int seed) {
    LabeledCloud lc;
    lc.numClasses = 4;
    std::mt19937 rng(seed);
    std::normal_distribution<float> gauss(0.0f, noise);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);

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
