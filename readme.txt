3D点云目标分割系统
版本：1.0
实现语言：C++17

一、简介

本程序为3D点云目标分割系统。程序可以读取PLY、XYZ、TXT格式的点云数据，
对点云建立KD树，估计法向量和曲率，并使用区域生长方法完成由粗到细的分割。
系统提供命令行程序和图形界面程序两种运行方式。

二、主要功能

1. 读取PLY、XYZ、TXT点云文件。
2. 使用KD树进行k近邻查询。
3. 使用PCA方法估计点云法向量和曲率。
4. 使用区域生长算法完成点云分割。
5. 支持多层分割，分割阈值由粗到细变化。
6. 支持将分割结果导出为彩色PLY文件。
7. 图形界面支持分割结果、法向量、高度和纯色显示。
8. 支持鼠标旋转、平移、缩放查看三维点云。

三、运行环境和依赖

1. 操作系统：Windows 10或Windows 11。
2. 编程语言：C++17。
3. 构建工具：CMake 3.16及以上版本。
4. 编译器：Visual Studio 2022、MinGW-w64或其他支持C++17的编译器。
5. 第三方库：
   (1) OpenGL：用于图形界面绘制。
   (2) GLFW：用于窗口创建和输入处理。
   (3) Dear ImGui：用于图形界面控件。
6. 项目中已包含third_party/glfw和third_party/imgui源码。
7. 项目中已包含tools/w64devkit、tools/cmake、tools/ninja，可直接配合build.bat构建。

四、文件说明

1. pcseg_cli.exe：命令行版本程序。
2. pcseg_gui.exe：图形界面版本程序。
3. pcseg_cli.bat：运行命令行演示。
4. pcseg_gui.bat：运行图形界面。
5. data目录：点云测试数据。
6. build.bat：Windows构建脚本。
7. package_windows.bat：生成可直接运行的dist目录。

五、运行方法

如果使用已经打包好的程序，进入以下目录：

    dist\PointCloudSegmentation_Run

双击运行：

    pcseg_gui.bat    打开图形界面
    pcseg_cli.bat    运行命令行演示

图形界面启动后，可在输入框中填写点云路径，例如：

    data\scene.ply

点击“载入文件”读取点云，点击“运行分割”执行分割，点击“导出当前分割”保存结果。

六、重新编译

在项目根目录运行：

    build.bat

构建完成后，程序位于：

    build\bin\pcseg_cli.exe
    build\bin\pcseg_gui.exe

常用构建命令：

    build.bat --clean          清理后重新构建
    build.bat --no-gui         只构建命令行程序和测试程序
    build.bat --debug          构建Debug版本
    build.bat --skip-tests     构建后不运行测试

如需重新生成可直接运行的目录，运行：

    package_windows.bat

七、命令行使用方法

1. 使用内置演示场景：

    pcseg_cli.exe --demo -o segmented_demo.ply

2. 读取点云文件并导出分割结果：

    pcseg_cli.exe data\scene.ply --levels 3 --coarse 30 --fine 10 -k 16 -o scene_result.ply

3. 常用参数：

    --demo             使用内置演示场景
    -o <文件>          输出PLY文件名
    -k <整数>          k近邻数量，默认16
    --levels <整数>    分割层数，默认3
    --coarse <度>      最粗层法向夹角阈值，默认30
    --fine <度>        最细层法向夹角阈值，默认10
    --curv <浮点数>    种子曲率阈值，默认0.08
    --min <整数>       最小段点数，默认30
    --level <整数>     导出指定层结果

八、输入输出格式

输入文件：
1. PLY文件：支持ascii和binary_little_endian格式，读取顶点的x、y、z坐标。
2. XYZ/TXT文件：每行至少包含三个浮点数，表示x、y、z坐标，多余列会被忽略。

输出文件：
1. 输出为ASCII格式PLY文件。
2. 每个点包含x、y、z、red、green、blue信息。
3. 不同分割段使用不同颜色显示，未分入有效段的点显示为灰色。

九、常见问题

1. 图形界面无法启动
检查显卡驱动和OpenGL支持情况。也可以运行build.bat --no-gui，只使用命令行版本。

2. 文件读取失败
检查点云文件路径是否正确，文件格式是否为PLY、XYZ或TXT。

3. 运行结果没有明显分割
可以调整k近邻数量、分割层数、法向夹角阈值和最小段点数后重新运行。

4. 重新编译失败
检查CMake和编译器是否可用，也可以直接使用项目自带的build.bat进行构建。
