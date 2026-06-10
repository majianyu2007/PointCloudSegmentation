#ifndef PCSEG_POINTCLOUDIO_H
#define PCSEG_POINTCLOUDIO_H

#include <string>
#include <vector>
#include "pcseg/PointCloud.h"
#include "pcseg/Vec3.h"

namespace pcseg {

// 点云文件读写。支持两种常见的纯文本/二进制格式：
//   .ply  —— 斯坦福点云格式，支持 ascii 和 binary_little_endian，读取 x/y/z（有的话也读法向量）
//   .xyz / .txt —— 每行 "x y z"（多余的列忽略）
//
// 读取成功返回 true；失败返回 false 并把原因写进 error。

// 按扩展名自动选择格式读取
bool loadPointCloud(const std::string& path, PointCloud& cloud, std::string& error);

bool loadPLY(const std::string& path, PointCloud& cloud, std::string& error);
bool loadXYZ(const std::string& path, PointCloud& cloud, std::string& error);

// 把点云连同每个点的颜色写成 ascii 的 .ply（用于把分割结果导出、再用别的软件查看）。
// colors 大小需与点数相同，分量取值 0~255。
bool savePLYColored(const std::string& path, const PointCloud& cloud,
                    const std::vector<unsigned char>& r,
                    const std::vector<unsigned char>& g,
                    const std::vector<unsigned char>& b,
                    std::string& error);

} // namespace pcseg

#endif // PCSEG_POINTCLOUDIO_H
