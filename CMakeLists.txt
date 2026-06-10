cmake_minimum_required(VERSION 3.16)
project(PointCloudSegmentation LANGUAGES CXX)

# 统一用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ----------------------------------------------------------------------------
# 核心库：点云数据结构 + KD树 + 法向量 + 分割算法 + 文件IO。纯 C++，不依赖任何 GUI。
# ----------------------------------------------------------------------------
add_library(pcseg_core STATIC
    core/src/PointCloud.cpp
    core/src/KdTree.cpp
    core/src/Normals.cpp
    core/src/Segmentation.cpp
    core/src/PointCloudIO.cpp
    core/src/Synthetic.cpp
)
target_include_directories(pcseg_core PUBLIC core/include)

# ----------------------------------------------------------------------------
# 命令行工具
# ----------------------------------------------------------------------------
add_executable(pcseg_cli apps/cli/main.cpp)
target_link_libraries(pcseg_cli PRIVATE pcseg_core)

# ----------------------------------------------------------------------------
# 单元测试（CTest）
# ----------------------------------------------------------------------------
enable_testing()
add_executable(pcseg_tests tests/test_main.cpp)
target_link_libraries(pcseg_tests PRIVATE pcseg_core)
add_test(NAME pcseg_tests COMMAND pcseg_tests)

# ----------------------------------------------------------------------------
# 图形界面（GLFW + OpenGL + Dear ImGui）。
# 只有在能找到 GLFW、且 third_party/imgui 存在时才编译，缺了不影响核心库和测试。
# ----------------------------------------------------------------------------
add_subdirectory(apps/gui)
