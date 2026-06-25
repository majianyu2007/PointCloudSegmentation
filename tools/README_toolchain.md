# 便携编译环境说明

项目已经提供 `dist/PointCloudSegmentation_Run` 运行目录生成脚本，老师演示时优先使用预编译 exe，不需要配置编译环境。

本项目支持把编译环境随源码一起带上。推荐目录结构如下：

```text
tools/
├── w64devkit/
│   └── bin/
│       ├── g++.exe
│       └── gcc.exe
├── cmake/
│   └── bin/
│       └── cmake.exe
└── ninja/
    └── ninja.exe
```

放好后直接运行：

```bat
build.bat
```

`build.bat` 会优先使用上述目录，然后再查找系统 PATH、CMake、CLion、CodeBlocks 等常见安装位置。

如果项目路径包含中文，MinGW 直接从中文路径启动时可能找不到自己的运行库。脚本会自动在 `%TEMP%\pcseg_toolchain_PointCloudSegmentation` 创建一个临时目录联接，让编译器从英文路径启动。工具链文件仍然保存在项目目录内。

注意：

- 完整便携工具链通常较大，本机项目专用工具链约 372 MB。
- 如果只放 CMake 和 Ninja，没有 C++ 编译器，仍然无法完成编译。
- 课程提交时更推荐同时提交预编译运行目录，让老师先双击运行；源码和构建脚本作为复查材料保留。
