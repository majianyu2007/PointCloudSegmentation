#include "pcseg/Primitive.h"
#include <cmath>

namespace pcseg {

namespace {
const float kPi = 3.14159265358979323846f;
}

void PlanePrimitive::sample(int count, std::mt19937& rng,
                            std::normal_distribution<float>& noise,
                            std::uniform_real_distribution<float>& uni,
                            PointCloud& cloud, std::vector<int>& labels) const {
    for (int i = 0; i < count; ++i) {
        float u = uni(rng);
        float v = uni(rng);
        Vec3 p = origin_ + du_ * u + dv_ * v;
        emit(p, rng, noise, cloud, labels);
    }
}

void SpherePrimitive::sample(int count, std::mt19937& rng,
                             std::normal_distribution<float>& noise,
                             std::uniform_real_distribution<float>& uni,
                             PointCloud& cloud, std::vector<int>& labels) const {
    for (int i = 0; i < count; ++i) {
        // 球面均匀采样
        float u = uni(rng);
        float v = uni(rng);
        float theta = 2.0f * kPi * u;
        float phi = std::acos(2.0f * v - 1.0f);
        Vec3 dir(std::sin(phi) * std::cos(theta),
                 std::sin(phi) * std::sin(theta),
                 std::cos(phi));
        emit(center_ + dir * radius_, rng, noise, cloud, labels);
    }
}

void CylinderPrimitive::sample(int count, std::mt19937& rng,
                               std::normal_distribution<float>& noise,
                               std::uniform_real_distribution<float>& uni,
                               PointCloud& cloud, std::vector<int>& labels) const {
    for (int i = 0; i < count; ++i) {
        float theta = 2.0f * kPi * uni(rng);
        float z = uni(rng) * height_;
        emit(Vec3(cx_ + radius_ * std::cos(theta),
                  cy_ + radius_ * std::sin(theta), z),
             rng, noise, cloud, labels);
    }
}

} // namespace pcseg
