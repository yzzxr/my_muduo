cmake_minimum_required(VERSION 3.2)
project(mymuduo)

# mymuduo最终编译成so动态库 设置动态库的路径 放置项目根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置调试信息,以及启动C++20语言标准,因为使用到了内联变量和unordered_map的contains成员函数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++20")

# 定义参与编译的源代码文件
file(GLOB; SRC_LIST; ${PROJECT_SOURCE_DIR}/*.cpp)

# 编译动态库
add_library(mymuduo SHARED ${SRC_LIST})