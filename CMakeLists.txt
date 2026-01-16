cmake_minimum_required(VERSION 3.10)
project(SAT_Raylib_Demo)

set(CMAKE_CXX_STANDARD 11)

# 查找 Raylib 包
# 如果你手动安装的 Raylib，可能需要设置 RAYLIB_PATH
# 例如：set(RAYLIB_PATH "C:/raylib/raylib/src")
# find_package(raylib CONFIG REQUIRED)

set(RAYLIB_REPO  "${CMAKE_CURRENT_SOURCE_DIR}/raylib")
set(RAYLIB_INCLUDE "${RAYLIB_REPO}/build/raylib/include")
set(RAYLIB_LIB     "${RAYLIB_REPO}/build/raylib/libraylib.a")

# 添加可执行文件
add_executable(sat_visualizer main.cc)
target_include_directories(sat_visualizer PRIVATE ${RAYLIB_INCLUDE})
target_link_libraries(sat_visualizer ${RAYLIB_LIB} GL m dl pthread X11)

# 链接 Raylib 库
# target_link_libraries(sat_visualizer PRIVATE raylib::raylib)

# 如果在 macOS 上遇到问题，可能需要添加这些
# if (APPLE)
#     target_link_libraries(sat_visualizer PRIVATE "-framework Cocoa" "-framework OpenGL" "-framework IOKit" "-framework CoreAudio" "-framework CoreVideo")
# endif()