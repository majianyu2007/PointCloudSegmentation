cmake_minimum_required(VERSION 3.16)
project(PointCloudSegmentation LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} ${CMAKE_BINARY_DIR}/bin)
endforeach()

option(PCSEG_BUILD_GUI "Build the OpenGL/ImGui viewer when dependencies are available" ON)
option(PCSEG_FETCH_GLFW "Download GLFW with CMake FetchContent when GLFW is not installed" ON)

function(pcseg_set_windows_runtime target_name)
    if(MINGW)
        target_link_options(${target_name} PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endfunction()

add_library(pcseg_core STATIC
    core/src/PointCloud.cpp
    core/src/KdTree.cpp
    core/src/Normals.cpp
    core/src/Segmentation.cpp
    core/src/PointCloudIO.cpp
    core/src/Primitive.cpp
    core/src/Synthetic.cpp
)
target_include_directories(pcseg_core PUBLIC core/include)

add_executable(pcseg_cli apps/cli/main.cpp)
target_link_libraries(pcseg_cli PRIVATE pcseg_core)
pcseg_set_windows_runtime(pcseg_cli)

enable_testing()
add_executable(pcseg_tests tests/test_main.cpp)
target_link_libraries(pcseg_tests PRIVATE pcseg_core)
pcseg_set_windows_runtime(pcseg_tests)
add_test(NAME pcseg_tests COMMAND pcseg_tests)

if(PCSEG_BUILD_GUI)
    add_subdirectory(apps/gui)
endif()

add_custom_target(pcseg_copy_runtime_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/pcseg_cli.bat
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/pcseg_cli.bat
    COMMENT "Copying runtime data and launcher scripts"
)
add_dependencies(pcseg_copy_runtime_files pcseg_cli)
if(TARGET pcseg_gui)
    add_custom_target(pcseg_copy_gui_launcher ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/pcseg_gui.bat
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/pcseg_gui.bat
        COMMENT "Copying GUI launcher script"
    )
    add_dependencies(pcseg_copy_gui_launcher pcseg_gui)
endif()
