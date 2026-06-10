// 命令行工具：读入点云 -> 估计法向量 -> 由粗到细分割 -> 打印统计 -> 导出着色点云。
// 没有图形界面也能完整跑通整套算法，方便测试和批处理。
//
// 用法示例：
//   pcseg_cli --demo -o out.ply             用内置演示场景跑一遍，结果写到 out.ply
//   pcseg_cli input.ply -o out.ply          读 input.ply
//   pcseg_cli input.xyz --levels 3 -k 16 --coarse 30 --fine 10 --min 30

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include "pcseg/PointCloud.h"
#include "pcseg/PointCloudIO.h"
#include "pcseg/KdTree.h"
#include "pcseg/Normals.h"
#include "pcseg/Segmentation.h"
#include "pcseg/Palette.h"
#include "pcseg/Synthetic.h"

using namespace pcseg;

static void printUsage() {
    std::cout <<
        "用法: pcseg_cli [输入文件|--demo] [选项]\n"
        "  --demo            使用内置演示场景（地面+球+圆柱+斜面）\n"
        "  -o <文件>         输出着色后的 ply（默认 segmented.ply）\n"
        "  -k <整数>         近邻数 k（默认 16）\n"
        "  --levels <整数>   由粗到细的层数（默认 3）\n"
        "  --coarse <度>     最粗层法向夹角阈值（默认 30）\n"
        "  --fine <度>       最细层法向夹角阈值（默认 10）\n"
        "  --curv <浮点>     种子曲率阈值（默认 0.08）\n"
        "  --min <整数>      最小段点数（默认 30）\n"
        "  --level <整数>    导出第几层的结果（默认最后一层）\n";
}

int main(int argc, char** argv) {
    std::string inputPath;
    std::string outputPath = "segmented.ply";
    bool useDemo = false;
    int exportLevel = -1;  // -1 表示最后一层
    SegParams params;

    // 简单地手工解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) { std::cerr << name << " 缺少参数\n"; std::exit(1); }
            return argv[++i];
        };
        if (a == "--demo") useDemo = true;
        else if (a == "-o") outputPath = next("-o");
        else if (a == "-k") params.k = std::stoi(next("-k"));
        else if (a == "--levels") params.levels = std::stoi(next("--levels"));
        else if (a == "--coarse") params.coarseSmoothnessDeg = std::stof(next("--coarse"));
        else if (a == "--fine") params.fineSmoothnessDeg = std::stof(next("--fine"));
        else if (a == "--curv") params.curvatureThreshold = std::stof(next("--curv"));
        else if (a == "--min") params.minClusterSize = std::stoi(next("--min"));
        else if (a == "--level") exportLevel = std::stoi(next("--level"));
        else if (a == "-h" || a == "--help") { printUsage(); return 0; }
        else if (!a.empty() && a[0] != '-') inputPath = a;
        else { std::cerr << "未知参数: " << a << "\n"; printUsage(); return 1; }
    }

    // 1) 准备点云
    PointCloud cloud;
    if (useDemo || inputPath.empty()) {
        std::cout << "[1/4] 生成内置演示场景...\n";
        LabeledCloud lc = makeScene();
        cloud = lc.cloud;
    } else {
        std::cout << "[1/4] 读取点云: " << inputPath << "\n";
        std::string err;
        if (!loadPointCloud(inputPath, cloud, err)) {
            std::cerr << "读取失败: " << err << "\n";
            return 1;
        }
    }
    std::cout << "      点数: " << cloud.size() << "\n";

    // 2) 建 KD 树 + 估计法向量
    std::cout << "[2/4] 建立 KD 树并估计法向量 (k=" << params.k << ") ...\n";
    auto t0 = std::chrono::steady_clock::now();
    KdTree tree;
    tree.build(cloud.points);
    estimateNormals(cloud, tree, params.k);

    // 3) 由粗到细分割
    std::cout << "[3/4] 由粗到细区域生长分割 (levels=" << params.levels
              << ", coarse=" << params.coarseSmoothnessDeg
              << ", fine=" << params.fineSmoothnessDeg << ") ...\n";
    SegResult result = segment(cloud, tree, params);
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // 打印每一层的段数
    std::cout << "      各层段数:";
    for (int L = 0; L < result.levels(); ++L) {
        std::cout << " L" << L << "=" << result.segmentCount[L];
    }
    std::cout << "\n      法向量+分割总耗时: " << ms << " ms\n";

    if (result.levels() == 0) {
        std::cerr << "分割没有产生结果\n";
        return 1;
    }

    // 4) 导出指定层的着色点云
    int lvl = (exportLevel < 0 || exportLevel >= result.levels())
                  ? result.levels() - 1 : exportLevel;
    const std::vector<int>& labels = result.levelLabels[lvl];

    std::vector<unsigned char> r(cloud.size()), g(cloud.size()), b(cloud.size());
    for (std::size_t i = 0; i < cloud.size(); ++i) {
        Vec3 c = colorForLabel(labels[i]);
        r[i] = (unsigned char)(c.x * 255);
        g[i] = (unsigned char)(c.y * 255);
        b[i] = (unsigned char)(c.z * 255);
    }
    std::string err;
    std::cout << "[4/4] 导出第 " << lvl << " 层结果到 " << outputPath << "\n";
    if (!savePLYColored(outputPath, cloud, r, g, b, err)) {
        std::cerr << "导出失败: " << err << "\n";
        return 1;
    }
    std::cout << "完成。\n";
    return 0;
}
