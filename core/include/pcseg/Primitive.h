#ifndef PCSEG_PRIMITIVE_H
#define PCSEG_PRIMITIVE_H

#include <memory>
#include <random>
#include <string>
#include <vector>
#include "pcseg/Vec3.h"
#include "pcseg/PointCloud.h"

namespace pcseg {

// 合成点云的几何体基类。派生类负责在各自表面采样点。
class Primitive {
public:
    explicit Primitive(int label) : label_(label) {}
    virtual ~Primitive() = default;

    int label() const { return label_; }

    virtual std::string name() const = 0;

    virtual void sample(int count, std::mt19937& rng,
                        std::normal_distribution<float>& noise,
                        std::uniform_real_distribution<float>& uni,
                        PointCloud& cloud, std::vector<int>& labels) const = 0;

protected:
    void emit(const Vec3& p, std::mt19937& rng,
              std::normal_distribution<float>& noise,
              PointCloud& cloud, std::vector<int>& labels) const {
        cloud.addPoint(Vec3(p.x + noise(rng), p.y + noise(rng), p.z + noise(rng)));
        labels.push_back(label_);
    }

    int label_;
};

// 平面：origin + du*u + dv*v，u、v 的范围为 [0,1]。
class PlanePrimitive : public Primitive {
public:
    PlanePrimitive(int label, const Vec3& origin, const Vec3& du, const Vec3& dv)
        : Primitive(label), origin_(origin), du_(du), dv_(dv) {}

    std::string name() const override { return "Plane"; }

    void sample(int count, std::mt19937& rng,
                std::normal_distribution<float>& noise,
                std::uniform_real_distribution<float>& uni,
                PointCloud& cloud, std::vector<int>& labels) const override;

private:
    Vec3 origin_, du_, dv_;
};

// 球面（球心 + 半径，均匀采样）
class SpherePrimitive : public Primitive {
public:
    SpherePrimitive(int label, const Vec3& center, float radius)
        : Primitive(label), center_(center), radius_(radius) {}

    std::string name() const override { return "Sphere"; }

    void sample(int count, std::mt19937& rng,
                std::normal_distribution<float>& noise,
                std::uniform_real_distribution<float>& uni,
                PointCloud& cloud, std::vector<int>& labels) const override;

private:
    Vec3 center_;
    float radius_;
};

// 竖直圆柱侧面（轴心 x,y + 半径 + 高）
class CylinderPrimitive : public Primitive {
public:
    CylinderPrimitive(int label, float cx, float cy, float radius, float height)
        : Primitive(label), cx_(cx), cy_(cy), radius_(radius), height_(height) {}

    std::string name() const override { return "Cylinder"; }

    void sample(int count, std::mt19937& rng,
                std::normal_distribution<float>& noise,
                std::uniform_real_distribution<float>& uni,
                PointCloud& cloud, std::vector<int>& labels) const override;

private:
    float cx_, cy_, radius_, height_;
};

} // namespace pcseg

#endif // PCSEG_PRIMITIVE_H
