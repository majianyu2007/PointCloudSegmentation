#ifndef PCSEG_GUI_CAMERA_H
#define PCSEG_GUI_CAMERA_H

#include "pcseg/Vec3.h"
#include "Mat4.h"

// 轨迹球(环绕)相机：相机始终盯着 target_，绕它转动、拉近拉远、平移。
// 点云的 +Z 朝上（地面在 z=0），所以 up 固定为 (0,0,1)。
class Camera {
public:
    void setTarget(const pcseg::Vec3& t) { target_ = t; }
    void setDistance(float d) { distance_ = d; }
    pcseg::Vec3 target() const { return target_; }
    float distance() const { return distance_; }

    // 鼠标拖动：绕竖直轴(yaw)和水平轴(pitch)转
    void orbit(float dYawDeg, float dPitchDeg) {
        yawDeg_ += dYawDeg;
        pitchDeg_ += dPitchDeg;
        if (pitchDeg_ > 89.0f) pitchDeg_ = 89.0f;     // 避免越过头顶产生翻转
        if (pitchDeg_ < -89.0f) pitchDeg_ = -89.0f;
    }

    // 滚轮缩放：factor>1 拉远，<1 拉近
    void zoom(float factor) {
        distance_ *= factor;
        if (distance_ < 0.01f) distance_ = 0.01f;
        if (distance_ > 1e5f) distance_ = 1e5f;
    }

    // 平移：在屏幕平面内移动观察目标（右键/中键拖动）
    void pan(float dxScreen, float dyScreen) {
        pcseg::Vec3 eye = eyePosition();
        pcseg::Vec3 forward = (target_ - eye).normalized();
        pcseg::Vec3 right = pcseg::cross(forward, pcseg::Vec3(0, 0, 1)).normalized();
        pcseg::Vec3 up = pcseg::cross(right, forward).normalized();
        float scale = distance_ * 0.0015f;   // 平移速度与距离成正比，手感更自然
        target_ += right * (-dxScreen * scale) + up * (dyScreen * scale);
    }

    // 预设视角
    void viewFront() { yawDeg_ = -90.0f; pitchDeg_ = 0.0f; }  // 正视：从 -Y 看向 +Y
    void viewSide()  { yawDeg_ = 0.0f;   pitchDeg_ = 0.0f; }  // 侧视：从 +X 看
    void viewTop()   { yawDeg_ = -90.0f; pitchDeg_ = 89.0f; } // 俯视：从上往下看

    pcseg::Vec3 eyePosition() const {
        const float d2r = 3.14159265358979f / 180.0f;
        float cp = std::cos(pitchDeg_ * d2r);
        pcseg::Vec3 dir(cp * std::cos(yawDeg_ * d2r),
                        cp * std::sin(yawDeg_ * d2r),
                        std::sin(pitchDeg_ * d2r));
        return target_ + dir * distance_;
    }

    Mat4 viewMatrix() const {
        return Mat4::lookAt(eyePosition(), target_, pcseg::Vec3(0, 0, 1));
    }

private:
    pcseg::Vec3 target_{0, 0, 0};
    float distance_ = 5.0f;
    float yawDeg_ = -60.0f;
    float pitchDeg_ = 25.0f;
};

#endif // PCSEG_GUI_CAMERA_H
