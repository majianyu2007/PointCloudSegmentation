#ifndef PCSEG_GUI_MAT4_H
#define PCSEG_GUI_MAT4_H

#include <cmath>
#include "pcseg/Vec3.h"

// 一个很小的 4x4 矩阵，列主序（和 OpenGL 一致），只提供本项目用得到的几个操作：
// 单位阵、相乘、透视投影、lookAt 视图矩阵。够画一个点云查看器用了。
struct Mat4 {
    float m[16];  // 列主序：m[col*4 + row]

    static Mat4 identity() {
        Mat4 r{};
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    // 矩阵相乘 this * b
    Mat4 operator*(const Mat4& b) const {
        Mat4 r{};
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k)
                    sum += m[k * 4 + row] * b.m[col * 4 + k];
                r.m[col * 4 + row] = sum;
            }
        }
        return r;
    }

    // 透视投影矩阵。fovy 为竖直视角（弧度）。
    static Mat4 perspective(float fovy, float aspect, float zNear, float zFar) {
        Mat4 r{};
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        float f = 1.0f / std::tan(fovy * 0.5f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (zFar + zNear) / (zNear - zFar);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
        return r;
    }

    // 视图矩阵：相机在 eye，看向 center，up 为大致上方向。
    static Mat4 lookAt(const pcseg::Vec3& eye, const pcseg::Vec3& center,
                       const pcseg::Vec3& up) {
        using pcseg::Vec3;
        Vec3 f = (center - eye).normalized();      // 前方
        Vec3 s = pcseg::cross(f, up).normalized(); // 右方
        Vec3 u = pcseg::cross(s, f);               // 重新算出的上方

        Mat4 r = identity();
        r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;
        r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
        r.m[12] = -pcseg::dot(s, eye);
        r.m[13] = -pcseg::dot(u, eye);
        r.m[14] =  pcseg::dot(f, eye);
        return r;
    }
};

#endif // PCSEG_GUI_MAT4_H
