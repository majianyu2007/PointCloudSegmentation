#ifndef PCSEG_SYNTHETIC_H
#define PCSEG_SYNTHETIC_H

#include <vector>
#include "pcseg/PointCloud.h"

namespace pcseg {

// 带"真值标签"的点云：每个点本来属于哪个几何体是已知的，
// 既能拿来做演示数据，也能在单元测试里当作分割结果的对照。
struct LabeledCloud {
    PointCloud cloud;
    std::vector<int> labels;  // 真值：每个点所属几何体编号
    int numClasses = 0;       // 几何体个数
};

// 两个互相垂直的平面（像墙角）。法向量差 90°，正确的分割应该把它们分成 2 块。
// 主要给单元测试用。noise 是叠加在坐标上的高斯噪声标准差。
LabeledCloud makeTwoPlanes(int perPlane = 800, float noise = 0.0f);

// 一个更丰富的演示场景：地面 + 球 + 圆柱 + 一块斜面，共 4 个几何体。
// 用来跑命令行工具和界面演示。seed 固定时结果可复现。
LabeledCloud makeScene(float noise = 0.004f, unsigned int seed = 42);

} // namespace pcseg

#endif // PCSEG_SYNTHETIC_H
