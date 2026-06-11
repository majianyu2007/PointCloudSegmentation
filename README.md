# 3D 点云目标分割系统

> 西北农林科技大学《面向对象程序设计实践》课程项目 —— 选题 56：3D点云目标分割方法设计与实现

读取 3D 点云文件，估计每个点的法向量与曲率，用**区域生长**算法对点云做**由粗到细**的目标分割，
并用 OpenGL 三维交互界面把原始点云、法向量、分割结果以及迭代细分的各个阶段可视化出来。

## 功能对照（题目要求）

| 题目要求 | 实现 |
|---|---|
| ① 需要图形学 / OpenGL 基础 | GLFW + OpenGL 3.3 + Dear ImGui 自绘三维视口与控制面板 |
| ② 读取 3D 点云文件并绘制 | 支持 `.ply`（ascii / binary_little_endian）与 `.xyz/.txt`；OpenGL 以 VBO 绘制带颜色的点 |
| ③ 由粗到细的迭代分割（考虑表面朝向、点的位置分布、局部形状连通性、片段一致性/差异性度量） | PCA 法向量(表面朝向) + KD 树 k 近邻(位置分布) + 区域生长(连通性) + 法向夹角/曲率阈值(一致性度量)；逐层收紧阈值实现由粗到细 |
| ④ 不同颜色显示不同片段，展示迭代分割各阶段 | 每段一种颜色；界面用滑块在"层"之间切换，回放由粗到细的细分过程 |

## 目录结构

```
PointCloudSegmentation/
├── core/                 # 核心算法库（纯 C++17，无 GUI 依赖，可单独测试）
│   ├── include/pcseg/    #   Vec3 / PointCloud / KdTree / Normals / Segmentation / PointCloudIO / Palette / Synthetic / Eigen3x3
│   └── src/
├── apps/
│   ├── cli/              # 命令行工具 pcseg_cli（无界面跑通全流程）
│   └── gui/              # 图形界面 pcseg_gui（GLFW+OpenGL+ImGui）
├── tests/                # 单元测试（CTest）
├── data/                 # 示例点云
├── docs/                 # 实施计划书、实习报告(LaTeX)
└── CMakeLists.txt
```

## 算法流程

```
点云 → 建 KD 树 → PCA 估计法向量+曲率 → 区域生长(第0层,粗) → 在每个段内收紧阈值再生长(第1层) → ...(更细) → 各层着色结果
```

区域生长：按曲率从小到大选种子，沿 k 近邻图向外扩张；邻居法向量与当前点夹角小于阈值才并入同一段，
只有足够平坦（曲率低于阈值）的点才继续作为扩张前沿——这样既保证片段内部朝向一致，又能在棱边处自然停下。

## 构建与运行

依赖：CMake ≥ 3.16、支持 C++17 的编译器。图形界面另需 GLFW 与 OpenGL（macOS 自带 OpenGL，
`brew install glfw`；ImGui 源码放在 `third_party/imgui/`）。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# 命令行：用内置演示场景跑一遍，导出着色点云
./build/pcseg_cli --demo -o segmented.ply

# 读取自己的点云
./build/pcseg_cli your_cloud.ply --levels 3 --coarse 30 --fine 10 -k 16 -o out.ply

# 图形界面（若已启用 GUI）
./build/apps/gui/pcseg_gui

# 跑单元测试
cd build && ctest --output-on-failure
```

### Windows / Code::Blocks

推荐仍然走 CMake，让它生成 Code::Blocks 工程文件，避免手工维护 `.cbp` 后源文件列表不同步。

1. 安装 MinGW-w64、CMake、Code::Blocks，以及 GLFW 开发库。
2. 确认 Code::Blocks 的编译器使用支持 C++17 的 MinGW-w64。
3. 在项目根目录生成工程：

```bat
cmake -S . -B build-codeblocks -G "CodeBlocks - MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

生成后打开 `build-codeblocks/PointCloudSegmentation.cbp` 即可。若本机暂未配置 GLFW，CMake 会跳过
`pcseg_gui`，但 `pcseg_core`、`pcseg_cli` 和 `pcseg_tests` 仍可构建。源码请保持 UTF-8 编码。

## 命令行参数

| 参数 | 含义 | 默认 |
|---|---|---|
| `--demo` | 使用内置演示场景（地面+球+圆柱+斜面） | — |
| `-k` | k 近邻数 | 16 |
| `--levels` | 由粗到细的层数 | 3 |
| `--coarse` / `--fine` | 最粗 / 最细层的法向夹角阈值（度） | 30 / 10 |
| `--curv` | 种子曲率阈值 | 0.08 |
| `--min` | 最小段点数（更小的段视为噪声） | 30 |
| `-o` | 输出 ply 路径 | segmented.ply |

## 课程交付物

- `docs/实施计划书.md` —— 实施计划书（附件1 内容）。
- `docs/report/` —— LaTeX 实习报告（nwafupaper 模板，XeLaTeX 编译，产物 `main.pdf`，26 页），
  编译方法见 `docs/report/README.md`。
- `docs/figures/` —— 图形界面实际渲染的分割结果与法向量插图。

## AI 使用说明

本项目在开发中使用了 Claude Code（Anthropic Claude Opus 4.8）辅助编码、调试与文档撰写；
算法选型、参数取值、测试断言与结果分析均经本人核对确定。详见实习报告“AI 使用情况”一节。

