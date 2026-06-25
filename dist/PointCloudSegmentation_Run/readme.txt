3D点云目标分割系统 运行说明

一、项目简介

本项目是《面向对象程序设计实践》课程设计，题目为“3D点云目标分割方法设计与实现”。
程序可以读取点云文件，估计每个点的法向量和曲率，并使用区域生长算法完成由粗到细的目标分割。
项目包含命令行程序 pcseg_cli 和图形界面程序 pcseg_gui。

二、开发环境和依赖

1. 编程语言：C++17
2. 构建工具：CMake 3.16 或以上
3. 推荐编译器：Visual Studio 2022、MinGW-w64 或其他支持 C++17 的编译器
4. 图形界面依赖：OpenGL、GLFW、Dear ImGui
5. Dear ImGui 源码已经放在 third_party/imgui 目录中
6. GLFW 源码已经放在 third_party/glfw 目录中，默认不需要联网下载
7. 项目内置便携编译环境：tools\w64devkit、tools\cmake、tools\ninja

如果机房电脑不能联网，或者图形环境配置不完整，可以执行：

    build.bat --no-gui

此时会构建核心算法库、命令行程序和测试程序，不构建图形界面。

三、快速构建

如果只需运行程序，请优先使用已经打包好的运行目录：

    dist\PointCloudSegmentation_Run

进入该目录后：

    双击 run_gui.bat   打开图形界面
    双击 run_demo.bat  运行命令行演示，生成 segmented_demo.ply

该运行目录已经包含 exe、data 和说明文档，不需要额外安装 CMake、配置 GLFW 或修改 PATH。

如果需要重新生成该运行目录，在项目根目录执行：

    package_windows.bat

在项目根目录打开命令行，执行：

    build.bat

构建脚本会自动完成 CMake 配置、编译和测试。
构建成功后，可执行文件位于：

    build\bin\pcseg_cli.exe
    build\bin\pcseg_gui.exe

常用构建命令：

    build.bat --clean          清理后重新构建
    build.bat --no-gui         只构建命令行程序和测试
    build.bat --debug          构建 Debug 版本
    build.bat --skip-tests     构建后不运行测试

build.bat 会优先使用项目目录中的便携编译环境：tools\w64devkit\bin、tools\cmake\bin、tools\ninja。项目路径含中文时，脚本会自动创建临时英文路径跳板，并优先使用 Ninja，以减少 MinGW 对中文路径的兼容问题。预编译运行目录仍建议一并提交，方便老师直接运行。

四、手动构建

如果不使用 build.bat，也可以执行：

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure

五、运行方法

1. 使用内置演示场景：

    build\bin\pcseg_cli.exe --demo -o build\segmented_demo.ply

2. 读取项目自带点云：

    build\bin\pcseg_cli.exe data\scene.ply --levels 3 --coarse 30 --fine 10 -k 16 -o build\scene_result.ply

3. 启动图形界面：

    build\bin\pcseg_gui.exe

4. 图形界面直接打开指定点云：

    build\bin\pcseg_gui.exe data\scene.ply

六、命令行参数

    --demo             使用内置演示场景
    -o <文件>          输出着色后的 PLY 文件，默认 segmented.ply
    -k <整数>          k 近邻数量，默认 16
    --levels <整数>    由粗到细的分割层数，默认 3
    --coarse <度>      最粗层法向夹角阈值，默认 30
    --fine <度>        最细层法向夹角阈值，默认 10
    --curv <浮点数>    种子曲率阈值，默认 0.08
    --min <整数>       最小段点数，默认 30
    --level <整数>     导出指定层结果，默认导出最后一层

七、输入输出格式

输入：
1. PLY 文件：支持 ascii 和 binary_little_endian，读取 x、y、z 坐标，若文件中有 nx、ny、nz 也会读取。
2. XYZ/TXT 文件：每行至少三个浮点数，表示 x y z，多余列会被忽略。

输出：
1. 输出为 ASCII PLY 文件。
2. 每个点包含 x、y、z、red、green、blue。
3. 不同分割段用不同颜色表示，噪声点显示为灰色。

八、主要功能

1. 读取 PLY、XYZ、TXT 点云文件。
2. 使用 KD 树进行 k 近邻查询。
3. 使用 PCA 方法估计法向量和曲率。
4. 使用区域生长算法分割点云。
5. 支持由粗到细的多层分割。
6. 支持分割结果导出为彩色 PLY。
7. 图形界面支持分割结果、法向量、高度和纯色显示。
8. 支持鼠标旋转、平移、缩放查看三维点云。

九、面向对象设计说明

1. PointCloud：保存点坐标、法向量和曲率。
2. KdTree：封装三维 KD 树和近邻查询。
3. Primitive：合成点云几何体抽象基类。
4. PlanePrimitive、SpherePrimitive、CylinderPrimitive：继承 Primitive，实现不同几何体采样。
5. Viewer：封装图形界面的点云载入、分割结果、相机和 OpenGL 绘制状态。
6. Camera：封装三维视角控制。

十、测试说明

测试程序为 build\bin\pcseg_tests.exe，也可以通过 CTest 运行：

    ctest --test-dir build -C Release --output-on-failure

测试内容包括 Vec3 运算、KD 树近邻查询、法向量估计、PLY 读写、区域生长分割、演示场景分割和多态采样。

十一、提交打包建议

提交源码时建议保留 apps、core、data、docs、tests、third_party/imgui、third_party/glfw、tools/w64devkit、tools/cmake、tools/ninja、CMakeLists.txt、build.bat、package_windows.bat、run_demo.bat、run_gui.bat、README.md、readme.txt。
打包前建议删除 build、build-ninja、build-windows、.git、.claude 等本地构建或临时目录。
如果需要提交可执行文件，建议运行 package_windows.bat，并把 dist\PointCloudSegmentation_Run 一并提交；不要提交完整 Debug 或 Release 构建目录。
