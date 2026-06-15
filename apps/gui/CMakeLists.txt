# 图形界面子工程。依赖 GLFW、系统 OpenGL，以及随仓库附带的 Dear ImGui 源码。
# 优先使用本机安装的 GLFW；找不到时可通过 FetchContent 自动下载。

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

set(PCSEG_GLFW_TARGET "")
set(PCSEG_GLFW_LIB "")
set(PCSEG_GLFW_INC "")

set(PCSEG_GLFW_HINTS)
if(DEFINED ENV{GLFW_ROOT})
    list(APPEND PCSEG_GLFW_HINTS "$ENV{GLFW_ROOT}")
endif()
if(DEFINED GLFW_ROOT)
    list(APPEND PCSEG_GLFW_HINTS "${GLFW_ROOT}")
endif()

find_package(glfw3 CONFIG QUIET)
if(TARGET glfw)
    set(PCSEG_GLFW_TARGET glfw)
elseif(TARGET glfw3)
    set(PCSEG_GLFW_TARGET glfw3)
else()
    find_library(PCSEG_GLFW_LIB NAMES glfw glfw3
        HINTS ${PCSEG_GLFW_HINTS}
        PATH_SUFFIXES lib lib64
        PATHS /usr/local/lib /usr/local/opt/glfw/lib /opt/homebrew/lib)
    find_path(PCSEG_GLFW_INC GLFW/glfw3.h
        HINTS ${PCSEG_GLFW_HINTS}
        PATH_SUFFIXES include
        PATHS /usr/local/include /usr/local/opt/glfw/include /opt/homebrew/include)
endif()

if(NOT PCSEG_GLFW_TARGET AND NOT (PCSEG_GLFW_LIB AND PCSEG_GLFW_INC) AND PCSEG_FETCH_GLFW)
    message(STATUS "未找到本机 GLFW，使用 FetchContent 下载 GLFW 3.4")
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE)
    FetchContent_MakeAvailable(glfw)
    if(TARGET glfw)
        set(PCSEG_GLFW_TARGET glfw)
    endif()
endif()

set(GUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GLCompat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewerPanel.cpp
)

if(EXISTS ${IMGUI_DIR}/imgui.cpp AND (PCSEG_GLFW_TARGET OR (PCSEG_GLFW_LIB AND PCSEG_GLFW_INC))
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
        ${IMGUI_DIR} ${IMGUI_DIR}/backends)
    if(WIN32)
        target_compile_definitions(pcseg_gui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    endif()
    if(PCSEG_GLFW_INC)
        target_include_directories(pcseg_gui PRIVATE ${PCSEG_GLFW_INC})
    endif()
    target_link_libraries(pcseg_gui PRIVATE pcseg_core)

    if(PCSEG_GLFW_TARGET)
        target_link_libraries(pcseg_gui PRIVATE ${PCSEG_GLFW_TARGET})
    else()
        target_link_libraries(pcseg_gui PRIVATE ${PCSEG_GLFW_LIB})
        if(WIN32)
            target_link_libraries(pcseg_gui PRIVATE gdi32 user32 shell32)
        endif()
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
