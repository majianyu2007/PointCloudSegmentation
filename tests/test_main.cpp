// 单元测试。不依赖第三方测试框架，用几个宏自己记录通过/失败数，
// 最后返回非零表示有测试失败（CTest 据此判定）。
//
// 覆盖：Vec3 基本运算、KD树近邻(对比暴力解)、平面法向量正确性、
//       PLY 读写往返、区域生长在两垂直平面上的分割、演示场景分割段数。

#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <string>
#include <memory>

#include "pcseg/Vec3.h"
#include "pcseg/PointCloud.h"
#include "pcseg/PointCloudIO.h"
#include "pcseg/KdTree.h"
#include "pcseg/Normals.h"
#include "pcseg/Segmentation.h"
#include "pcseg/Synthetic.h"
#include "pcseg/Primitive.h"

using namespace pcseg;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond) do {                                              \
    ++g_checks;                                                       \
    if (!(cond)) {                                                    \
        ++g_failures;                                                 \
        std::cerr << "  [FAIL] " << __LINE__ << ": " #cond "\n";      \
    }                                                                 \
} while (0)

#define CHECK_NEAR(a, b, eps) do {                                    \
    ++g_checks;                                                       \
    double _d = std::fabs((double)(a) - (double)(b));                 \
    if (_d > (eps)) {                                                 \
        ++g_failures;                                                 \
        std::cerr << "  [FAIL] " << __LINE__ << ": |" << (a) << " - " \
                  << (b) << "| = " << _d << " > " << (eps) << "\n";   \
    }                                                                 \
} while (0)

// ---------------------------------------------------------------------------
static void testVec3() {
    std::cout << "[test] Vec3 基本运算\n";
    Vec3 a(1, 2, 2), b(0, 0, 1);
    CHECK_NEAR(a.length(), 3.0f, 1e-5);
    CHECK_NEAR(dot(a, b), 2.0f, 1e-5);
    Vec3 c = cross(Vec3(1, 0, 0), Vec3(0, 1, 0));
    CHECK_NEAR(c.x, 0.0f, 1e-6); CHECK_NEAR(c.y, 0.0f, 1e-6); CHECK_NEAR(c.z, 1.0f, 1e-6);
    Vec3 n = a.normalized();
    CHECK_NEAR(n.length(), 1.0f, 1e-5);
}

// KD 树的 kNN 结果应该和暴力遍历一致
static void testKdTree() {
    std::cout << "[test] KD 树 k 近邻 (对比暴力解)\n";
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> uni(-1, 1);
    std::vector<Vec3> pts;
    for (int i = 0; i < 500; ++i) pts.push_back(Vec3(uni(rng), uni(rng), uni(rng)));

    KdTree tree;
    tree.build(pts);

    int k = 8;
    for (int q = 0; q < 20; ++q) {
        Vec3 query = pts[q * 10];
        std::vector<int> got = tree.kNearest(query, k);

        // 暴力解：按距离排序取前 k 个
        std::vector<std::pair<float, int>> all;
        for (int i = 0; i < (int)pts.size(); ++i)
            all.push_back({distanceSquared(query, pts[i]), i});
        std::sort(all.begin(), all.end());

        CHECK((int)got.size() == k);
        // 比较两边返回的点集合（下标集合应一致）
        std::vector<int> expect;
        for (int i = 0; i < k; ++i) expect.push_back(all[i].second);
        std::sort(got.begin(), got.end());
        std::sort(expect.begin(), expect.end());
        CHECK(got == expect);
    }
}

// 平面上的点，法向量应当垂直于平面、曲率应当很小
static void testNormalsOnPlane() {
    std::cout << "[test] 平面法向量 + 曲率\n";
    PointCloud cloud;
    for (int i = 0; i < 40; ++i)
        for (int j = 0; j < 40; ++j)
            cloud.addPoint(Vec3(i * 0.05f, j * 0.05f, 0.0f));  // z=0 平面

    KdTree tree;
    tree.build(cloud.points);
    estimateNormals(cloud, tree, 16);

    CHECK(cloud.hasNormals());
    // 取中间一点（避开边界），法向量应接近 (0,0,1)
    int mid = 20 * 40 + 20;
    CHECK_NEAR(std::fabs(cloud.normals[mid].z), 1.0f, 0.05);
    CHECK(cloud.curvature[mid] < 0.02f);   // 平面非常平坦
}

// PLY 写出再读回，点坐标应一致
static void testPlyRoundTrip() {
    std::cout << "[test] PLY 着色写出 + 读回\n";
    PointCloud cloud;
    cloud.addPoint(Vec3(1.5f, -2.0f, 3.25f));
    cloud.addPoint(Vec3(0.0f, 0.0f, 0.0f));
    cloud.addPoint(Vec3(-10.0f, 5.5f, 100.0f));
    std::vector<unsigned char> r{255, 0, 0}, g{0, 255, 0}, b{0, 0, 255};

    std::string err;
    const std::string path = "test_roundtrip.ply";
    CHECK(savePLYColored(path, cloud, r, g, b, err));

    PointCloud loaded;
    CHECK(loadPLY(path, loaded, err));
    CHECK(loaded.size() == cloud.size());
    if (loaded.size() == cloud.size()) {
        for (std::size_t i = 0; i < cloud.size(); ++i) {
            CHECK_NEAR(loaded.points[i].x, cloud.points[i].x, 1e-3);
            CHECK_NEAR(loaded.points[i].y, cloud.points[i].y, 1e-3);
            CHECK_NEAR(loaded.points[i].z, cloud.points[i].z, 1e-3);
        }
    }
}

// 计算分割"纯度"：每个真值类别里，落到同一个分割段的最多点数 / 总点数
static double purity(const std::vector<int>& gt, const std::vector<int>& seg,
                     int numClasses) {
    int correct = 0;
    for (int c = 0; c < numClasses; ++c) {
        std::vector<int> hist;  // 段号 -> 计数
        for (std::size_t i = 0; i < gt.size(); ++i) {
            if (gt[i] != c) continue;
            int s = seg[i];
            if (s < 0) continue;
            if (s >= (int)hist.size()) hist.resize(s + 1, 0);
            hist[s]++;
        }
        if (!hist.empty()) correct += *std::max_element(hist.begin(), hist.end());
    }
    return (double)correct / (double)gt.size();
}

// 两个互相垂直的平面，应该被分成 2 块，且分得很干净
static void testTwoPlanes() {
    std::cout << "[test] 区域生长：两垂直平面应分成 2 段\n";
    LabeledCloud lc = makeTwoPlanes(800, 0.0f);
    KdTree tree;
    tree.build(lc.cloud.points);
    estimateNormals(lc.cloud, tree, 16);

    SegParams params;
    params.levels = 1;             // 只看第 0 层（最粗）就够分开 90° 的两个面
    params.minClusterSize = 20;
    SegResult res = segment(lc.cloud, tree, params);

    CHECK(res.levels() == 1);
    // 两个朝向不同、且空间分离的平面，应当干净地分成 2 段
    std::cout << "      段数 = " << res.segmentCount[0] << "\n";
    CHECK(res.segmentCount[0] == 2);
    double p = purity(lc.labels, res.levelLabels[0], lc.numClasses);
    std::cout << "      纯度 = " << p << "\n";
    CHECK(p > 0.95);
}

// 演示场景（地面+球+圆柱+斜面）由粗到细，越细段数应不少于粗的，且最细层至少分出 4 块
static void testScene() {
    std::cout << "[test] 演示场景由粗到细分割\n";
    LabeledCloud lc = makeScene(0.004f);
    KdTree tree;
    tree.build(lc.cloud.points);
    estimateNormals(lc.cloud, tree, 16);

    SegParams params;
    params.levels = 3;
    SegResult res = segment(lc.cloud, tree, params);

    CHECK(res.levels() == 3);
    // 由粗到细：后一层段数 >= 前一层（细分只会更多或持平）
    CHECK(res.segmentCount[1] >= res.segmentCount[0]);
    CHECK(res.segmentCount[2] >= res.segmentCount[1]);
    std::cout << "      段数: " << res.segmentCount[0] << " -> "
              << res.segmentCount[1] << " -> " << res.segmentCount[2] << "\n";
    // 地面、球、圆柱、斜面，最细层至少能分出 4 个面
    CHECK(res.segmentCount[2] >= 4);
    double p = purity(lc.labels, res.levelLabels[2], lc.numClasses);
    std::cout << "      最细层纯度 = " << p << "\n";
    CHECK(p > 0.85);
}

// 多态：通过基类 Primitive 指针调用派生类（Plane/Sphere/Cylinder）的虚函数
static void testPrimitivePolymorphism() {
    std::cout << "[test] 多态：Primitive 基类指针调用派生类虚函数\n";
    std::mt19937 rng(1);
    std::normal_distribution<float> noise(0.0f, 0.0f);  // 不加噪声，便于精确断言
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);

    std::vector<std::unique_ptr<Primitive>> prims;
    prims.push_back(std::make_unique<PlanePrimitive>(
        0, Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0)));   // z=0 平面
    prims.push_back(std::make_unique<SpherePrimitive>(
        1, Vec3(0, 0, 0), 1.0f));                            // 单位球
    prims.push_back(std::make_unique<CylinderPrimitive>(
        2, 0.0f, 0.0f, 1.0f, 2.0f));                         // 半径1圆柱

    // 虚函数 name() 应按实际类型分派
    CHECK(prims[0]->name() == "Plane");
    CHECK(prims[1]->name() == "Sphere");
    CHECK(prims[2]->name() == "Cylinder");

    PointCloud c;
    std::vector<int> labels;
    for (const auto& p : prims) p->sample(50, rng, noise, uni, c, labels);

    CHECK(c.size() == 150);
    CHECK(labels.size() == 150);
    // 标签按几何体顺序写入
    CHECK(labels[0] == 0 && labels[70] == 1 && labels[130] == 2);
    // 平面点 z 应为 0（噪声为 0）
    CHECK(std::fabs(c.points[10].z) < 1e-6);
    // 球面点到球心距离应等于半径 1
    CHECK_NEAR((c.points[70] - Vec3(0, 0, 0)).length(), 1.0f, 1e-4);
    // 圆柱侧面点到轴线(x=y=0)的水平距离应等于半径 1
    float r = std::sqrt(c.points[130].x * c.points[130].x +
                        c.points[130].y * c.points[130].y);
    CHECK_NEAR(r, 1.0f, 1e-4);
}

int main() {
    std::cout << "==== pcseg 单元测试 ====\n";
    testVec3();
    testKdTree();
    testNormalsOnPlane();
    testPlyRoundTrip();
    testPrimitivePolymorphism();
    testTwoPlanes();
    testScene();

    std::cout << "========================\n";
    std::cout << "检查项: " << g_checks << "，失败: " << g_failures << "\n";
    if (g_failures == 0) {
        std::cout << "全部通过 ✓\n";
        return 0;
    }
    std::cout << "存在失败 ✗\n";
    return 1;
}
