#include "pcseg/PointCloudIO.h"

#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace pcseg {

// 把字符串转成小写，用来判断扩展名
static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

bool loadPointCloud(const std::string& path, PointCloud& cloud, std::string& error) {
    std::string lower = toLower(path);
    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".ply") {
        return loadPLY(path, cloud, error);
    }
    // 其余当作纯文本 xyz 处理（.xyz / .txt / 无扩展名）
    return loadXYZ(path, cloud, error);
}

bool loadXYZ(const std::string& path, PointCloud& cloud, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "无法打开文件: " + path;
        return false;
    }
    cloud.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;  // 跳过空行和注释
        std::istringstream ss(line);
        float x, y, z;
        if (ss >> x >> y >> z) {
            cloud.addPoint(Vec3(x, y, z));
        }
    }
    if (cloud.empty()) {
        error = "文件中没有读到任何点: " + path;
        return false;
    }
    return true;
}

// --- PLY 读取 ---
// PLY 文件分头部（ascii）和数据体（ascii 或 binary_little_endian）。
// 头部声明每个 vertex 有哪些属性（property），我们关心 x/y/z，顺便支持 nx/ny/nz。
bool loadPLY(const std::string& path, PointCloud& cloud, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "无法打开文件: " + path;
        return false;
    }
    cloud.clear();

    std::string line;
    // 第一行必须是 "ply"
    std::getline(in, line);
    if (line.substr(0, 3) != "ply") {
        error = "不是合法的 PLY 文件: " + path;
        return false;
    }

    bool binaryLE = false;       // 数据体是否为小端二进制
    int vertexCount = 0;
    // 记录 vertex 的属性顺序，用来定位 x/y/z/nx/ny/nz 是第几个 float
    std::vector<std::string> props;
    bool inVertex = false;

    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;
        if (token == "format") {
            std::string fmt;
            ss >> fmt;
            if (fmt == "binary_little_endian") binaryLE = true;
            else if (fmt == "binary_big_endian") {
                error = "暂不支持 big_endian 的 PLY";
                return false;
            }
        } else if (token == "element") {
            std::string name;
            int cnt;
            ss >> name >> cnt;
            if (name == "vertex") {
                vertexCount = cnt;
                inVertex = true;
            } else {
                inVertex = false;
            }
        } else if (token == "property") {
            if (inVertex) {
                std::string type, name;
                ss >> type >> name;
                // 简化：假设 x/y/z/nx/ny/nz 都是 float 类型（最常见）
                props.push_back(name);
            }
        } else if (token == "end_header") {
            break;
        }
    }

    // 找到 x/y/z（以及可选 nx/ny/nz）在属性列表里的位置
    auto indexOf = [&](const std::string& name) -> int {
        for (int i = 0; i < (int)props.size(); ++i)
            if (props[i] == name) return i;
        return -1;
    };
    int ix = indexOf("x"), iy = indexOf("y"), iz = indexOf("z");
    int inx = indexOf("nx"), iny = indexOf("ny"), inz = indexOf("nz");
    if (ix < 0 || iy < 0 || iz < 0) {
        error = "PLY 头部缺少 x/y/z 属性";
        return false;
    }
    bool hasNormals = (inx >= 0 && iny >= 0 && inz >= 0);
    int nProps = (int)props.size();

    cloud.points.reserve(vertexCount);
    if (hasNormals) cloud.normals.reserve(vertexCount);

    if (binaryLE) {
        // 二进制：每个顶点 nProps 个 4 字节 float，紧挨着排列
        std::vector<float> buf(nProps);
        for (int v = 0; v < vertexCount; ++v) {
            in.read(reinterpret_cast<char*>(buf.data()), nProps * sizeof(float));
            if (!in) {
                error = "PLY 二进制数据读取中断";
                return false;
            }
            cloud.points.push_back(Vec3(buf[ix], buf[iy], buf[iz]));
            if (hasNormals) cloud.normals.push_back(Vec3(buf[inx], buf[iny], buf[inz]));
        }
    } else {
        // ascii：每行一个顶点，空格分隔
        for (int v = 0; v < vertexCount; ++v) {
            if (!std::getline(in, line)) {
                error = "PLY ascii 数据行数不足";
                return false;
            }
            std::istringstream ss(line);
            std::vector<float> vals;
            float f;
            while (ss >> f) vals.push_back(f);
            if ((int)vals.size() < nProps) continue;
            cloud.points.push_back(Vec3(vals[ix], vals[iy], vals[iz]));
            if (hasNormals) cloud.normals.push_back(Vec3(vals[inx], vals[iny], vals[inz]));
        }
    }

    if (cloud.empty()) {
        error = "PLY 中没有读到任何点";
        return false;
    }
    return true;
}

bool savePLYColored(const std::string& path, const PointCloud& cloud,
                    const std::vector<unsigned char>& r,
                    const std::vector<unsigned char>& g,
                    const std::vector<unsigned char>& b,
                    std::string& error) {
    if (r.size() != cloud.size() || g.size() != cloud.size() || b.size() != cloud.size()) {
        error = "颜色数组大小与点数不一致";
        return false;
    }
    std::ofstream out(path);
    if (!out) {
        error = "无法写入文件: " + path;
        return false;
    }
    // 写 ascii PLY 头部
    out << "ply\n";
    out << "format ascii 1.0\n";
    out << "element vertex " << cloud.size() << "\n";
    out << "property float x\n";
    out << "property float y\n";
    out << "property float z\n";
    out << "property uchar red\n";
    out << "property uchar green\n";
    out << "property uchar blue\n";
    out << "end_header\n";
    for (std::size_t i = 0; i < cloud.size(); ++i) {
        const Vec3& p = cloud.points[i];
        out << p.x << " " << p.y << " " << p.z << " "
            << (int)r[i] << " " << (int)g[i] << " " << (int)b[i] << "\n";
    }
    return true;
}

} // namespace pcseg
