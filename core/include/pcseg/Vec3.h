#ifndef PCSEG_VEC3_H
#define PCSEG_VEC3_H

#include <cmath>

namespace pcseg {

// 三维向量 / 点。整个工程里点坐标、法向量都用它表示。
// 写成 struct + 内联运算符，方便像数学公式一样使用：a + b、a - b、dot(a,b) 等。
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }

    // 平方长度（避免开方，比较距离时更快）
    float lengthSquared() const { return x * x + y * y + z * z; }
    // 长度（模）
    float length() const { return std::sqrt(lengthSquared()); }

    // 单位化。零向量直接返回自身，避免除以 0。
    Vec3 normalized() const {
        float len = length();
        if (len < 1e-12f) return *this;
        return *this / len;
    }
};

// 点积
inline float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 叉积
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

// 两点间距离的平方
inline float distanceSquared(const Vec3& a, const Vec3& b) {
    return (a - b).lengthSquared();
}

} // namespace pcseg

#endif // PCSEG_VEC3_H
