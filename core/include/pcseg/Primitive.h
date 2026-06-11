#ifndef PCSEG_PRIMITIVE_H
#define PCSEG_PRIMITIVE_H

#include <memory>
#include <random>
#include <string>
#include <vector>
#include "pcseg/Vec3.h"
#include "pcseg/PointCloud.h"

namespace pcseg {

// 几何体抽象基类。
// 合成点云里的每一种形状（平面、球、圆柱……）都是一个 Primitive。
// 这里用面向对象的“抽象基类 + 虚函数”来组织：基类规定接口（在表面采样若干点、
// 报出名称），各派生类给出自己的实现；makeScene 只通过基类指针调用 sample()，
// 在运行时由多态决定实际采样哪种几何体——这样新增一种形状只需再派生一个子类，
// 不必改动场景生成代码。
class Primitive {
public:
    explicit Primitive(int label) : label_(label) {}
    virtual ~Primitive() = default;

    int label() const { return label_; }

    // 几何体名称（纯虚函数，子类必须实现）
    virtual std::string name() const = 0;

    // 在该几何体表面采样 count 个点，叠加高斯噪声后追加到 cloud，
    // 同时把真值标签写入 labels。（纯虚函数，多态的核心）
    // 传入共享的随机引擎与分布对象，保证整场景的随机序列连续、可复现。
    virtual void sample(int count, std::mt19937& rng,
                        std::normal_distribution<float>& noise,
                        std::uniform_real_distribution<float>& uni,
                        PointCloud& cloud, std::vector<int>& labels) const = 0;

protected:
    // 派生类公用：把一个点叠加噪声后写入点云与标签
    void emit(const Vec3& p, std::mt19937& rng,
              std::normal_distribution<float>& noise,
              PointCloud& cloud, std::vector<int>& labels) const {
        cloud.addPoint(Vec3(p.x + noise(rng), p.y + noise(rng), p.z + noise(rng)));
        labels.push_back(label_);
    }

    int label_;
};

// 平面（参数化为 origin + du*u + dv*v，u,v in [0,1]）。
// 地面、斜面都用它，只是 origin/du/dv 不同——这就是“同一个类、不同对象”。
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
