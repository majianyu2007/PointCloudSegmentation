# 3D 点云目标分割系统

西北农林科技大学《面向对象程序设计实践》课程项目，题目为“3D 点云目标分割方法设计与实现”。

程序读取 `.ply`、`.xyz` 或 `.txt` 点云，先用 KD 树查找近邻，再通过 PCA 估计法向量和曲率，最后使用区域生长算法完成由粗到细的点云分割。项目同时提供命令行程序和 OpenGL 图形界面，便于批处理、测试和课堂演示。

## 功能

| 项目要求 | 实现情况 |
|---|---|
| C++ 多文件工程 | `core`、`apps/cli`、`apps/gui`、`tests` 分模块组织 |
| 面向对象设计 | `PointCloud`、`KdTree`、`Primitive`、`PlanePrimitive`、`SpherePrimitive`、`CylinderPrimitive`、`Viewer` 等类 |
| 封装、包含、继承、多态 | 合成点云使用 `Primitive` 抽象基类和派生类；GUI 封装渲染、相机和交互状态 |
| 文件读写 | 支持读取 PLY / XYZ / TXT，输出带 RGB 颜色的 PLY |
| 图形显示 | GLFW + OpenGL + Dear ImGui 显示原始点云、法向量、高度着色和分割结果 |
| 测试数据 | `data/` 下包含演示点云、压力测试点云和真实公开点云样例 |
| 自动测试 | `tests/test_main.cpp` 覆盖向量运算、KD 树、法向量、PLY 读写、区域生长和多态采样 |

项目自有 C++ 源码约 2000 行，不包含 `third_party/imgui`。

## 目录结构

```text
PointCloudSegmentation/
├── apps/
│   ├── cli/              命令行程序 pcseg_cli
│   └── gui/              图形界面 pcseg_gui
├── core/
│   ├── include/pcseg/    核心类和算法头文件
│   └── src/              核心类和算法实现
├── data/                 点云数据
├── docs/                 实施计划、报告和截图
├── tests/                CTest 单元测试
├── third_party/imgui/    Dear ImGui 源码
├── CMakeLists.txt
├── build.bat             Windows 构建脚本
└── readme.txt            运行与提交说明
```

## 环境依赖

基础构建需要：

- CMake 3.16 或以上
- 支持 C++17 的编译器，如 Visual Studio 2022、MinGW-w64 或 clang

图形界面额外需要：

- OpenGL
- GLFW 3
- Dear ImGui

`third_party/imgui` 和 `third_party/glfw` 已随项目提供。CMake 会优先使用本地 GLFW 源码，不需要联网下载；如果本地 GLFW 目录不存在，才会尝试查找系统 GLFW 或通过 FetchContent 下载。

## Windows 构建

如需免配置运行，可先生成独立运行目录：

```bat
package_windows.bat
```

脚本会生成：

```text
dist\PointCloudSegmentation_Run\
├── pcseg_cli.exe
├── pcseg_gui.exe
├── pcseg_cli.bat
├── pcseg_gui.bat
├── data\
├── readme.txt
└── README.md
```

可直接双击 `pcseg_gui.bat` 打开图形界面，或双击 `pcseg_cli.bat` 运行命令行演示并生成 `segmented_demo.ply`。这个目录不需要额外安装 CMake、配置 GLFW 或调整 PATH。

最简单的方式是在项目根目录双击或执行：

```bat
build.bat
```

常用选项：

```bat
build.bat --clean          清理后重新构建
build.bat --no-gui         不构建图形界面，避免 GLFW 下载或 OpenGL 环境问题
build.bat --debug          构建 Debug 版本
build.bat --skip-tests     构建后不运行测试
```

构建成功后，可执行文件在：

```text
build\bin\pcseg_cli.exe
build\bin\pcseg_gui.exe
```

构建脚本会同时复制 `data` 到 `build\bin\data`，并在 `build\bin` 生成 `pcseg_cli.bat` 与 `pcseg_gui.bat`，可直接从该目录双击运行。

脚本会自动查找常见位置的 CMake 和 Ninja。在中文路径下会优先使用 Ninja，避免 MinGW Makefiles 对中文路径兼容不稳定的问题。

本项目也带有便携编译环境。`build.bat` 会优先使用 `tools\w64devkit\bin`、`tools\cmake\bin` 和 `tools\ninja`，即使当前系统没有预先配置 C++ 编译环境，也可以直接运行 `build.bat`。项目路径包含中文时，脚本会自动创建临时英文路径跳板，避免 MinGW 对中文路径解析不稳定。

## 手动构建

```bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

只构建命令行版本：

```bat
cmake -S . -B build -DPCSEG_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Code::Blocks 用户可以生成工程文件：

```bat
cmake -S . -B build-codeblocks -G "CodeBlocks - MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

## 运行示例

使用内置演示场景并导出分割结果：

```bat
build\bin\pcseg_cli.exe --demo -o build\segmented_demo.ply
```

读取项目自带点云：

```bat
build\bin\pcseg_cli.exe data\scene.ply --levels 3 --coarse 30 --fine 10 -k 16 -o build\scene_result.ply
```

打开图形界面：

```bat
build\bin\pcseg_gui.exe
```

打开指定点云：

```bat
build\bin\pcseg_gui.exe data\scene.ply
```

## 命令行参数

| 参数 | 含义 | 默认值 |
|---|---|---|
| `--demo` | 使用内置演示场景 | 未启用 |
| `-o <文件>` | 输出着色 PLY 文件 | `segmented.ply` |
| `-k <整数>` | 近邻数 | `16` |
| `--levels <整数>` | 由粗到细的分割层数 | `3` |
| `--coarse <度>` | 最粗层法向夹角阈值 | `30` |
| `--fine <度>` | 最细层法向夹角阈值 | `10` |
| `--curv <浮点>` | 种子曲率阈值 | `0.08` |
| `--min <整数>` | 最小段点数 | `30` |
| `--level <整数>` | 导出指定层结果 | 最后一层 |

## 输入输出

输入格式：

- `.ply`：支持 `ascii` 和 `binary_little_endian`，读取顶点的 `x y z`，若存在 `nx ny nz` 也会读取。
- `.xyz` / `.txt`：每行至少包含三个浮点数，表示 `x y z`，多余列会被忽略。

输出格式：

- ASCII PLY，包含 `x y z red green blue`。不同分割段使用不同颜色，噪声点为灰色。

## 算法流程

```text
读取点云
  -> 构建 KD 树
  -> PCA 估计法向量和曲率
  -> 按曲率选取种子点
  -> 在 k 近邻图上做区域生长
  -> 逐层收紧法向夹角阈值
  -> 导出或显示分割结果
```

区域生长时，邻域点的法向夹角小于阈值才会合并到当前段；曲率较小的点继续作为扩张前沿。这样可以在平滑表面内保持连通，又能在棱边或形状边界处停止扩张。

## 提交说明

源码提交时建议保留：

- `apps/`
- `core/`
- `data/`
- `docs/`
- `tests/`
- `third_party/imgui/`
- `third_party/glfw/`
- `tools/w64devkit/`
- `tools/cmake/`
- `tools/ninja/`
- `CMakeLists.txt`
- `build.bat`
- `package_windows.bat`
- `pcseg_cli.bat`
- `pcseg_gui.bat`
- `README.md`
- `readme.txt`

打包前建议删除本地构建目录和临时目录，例如 `build/`、`build-ninja/`、`build-windows/`、`.git/`、`.claude/`。如果需要提交可执行文件，建议运行 `package_windows.bat`，把 `dist\PointCloudSegmentation_Run` 作为可直接运行版本一并提交，不要把完整 Debug / Release 构建目录一起打包。
