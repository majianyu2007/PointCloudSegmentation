# 图形界面子工程。依赖 GLFW、系统 OpenGL，以及随仓库附带的 Dear ImGui 源码。
# 任一缺失就跳过 GUI（核心库、命令行、测试仍可正常构建）。

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

# 找 GLFW：优先用 Homebrew 安装的版本
find_package(glfw3 QUIET)
find_library(GLFW_LIB NAMES glfw glfw3
    PATHS /usr/local/lib /usr/local/opt/glfw/lib /opt/homebrew/lib)
find_path(GLFW_INC GLFW/glfw3.h
    PATHS /usr/local/include /usr/local/opt/glfw/include /opt/homebrew/include)

set(GUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer.cpp
)

if(EXISTS ${IMGUI_DIR}/imgui.cpp AND (TARGET glfw OR GLFW_LIB) AND GLFW_INC
   AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
    message(STATUS "找到 GLFW 与 ImGui，启用图形界面 pcseg_gui")

    add_executable(pcseg_gui
        ${GUI_SOURCES}
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(pcseg_gui PRIVATE
        ${IMGUI_DIR} ${IMGUI_DIR}/backends ${GLFW_INC})
    target_link_libraries(pcseg_gui PRIVATE pcseg_core)

    if(TARGET glfw)
        target_link_libraries(pcseg_gui PRIVATE glfw)
    else()
        target_link_libraries(pcseg_gui PRIVATE ${GLFW_LIB})
    endif()

    if(APPLE)
        target_compile_definitions(pcseg_gui PRIVATE GL_SILENCE_DEPRECATION)
        target_link_libraries(pcseg_gui PRIVATE
            "-framework OpenGL" "-framework Cocoa"
            "-framework IOKit" "-framework CoreVideo")
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(pcseg_gui PRIVATE OpenGL::GL)
    endif()
else()
    message(STATUS "未找到 GLFW 或 ImGui（或 GUI 源码缺失），跳过 pcseg_gui")
endif()
