cmake_minimum_required(VERSION 2.8)

# 生成的项目名
project(mpp_test)

# 设置编译工具
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER "arm-himix100-linux-gcc")
set(CMAKE_CXX_COMPILER "arm-himix100-linux-g++")

# 设置头文件路径
include_directories(../include)
include_directories(../include/hisi)

# 设置链接库路径
link_directories(../../bin)
# 链接库
link_libraries(mpi securec upvqe VoiceEngine dnvqe)
link_libraries(pthread)

#设置项目生成目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../bin)

# 包含源文件
aux_source_directory(./ DIR_SRCS)

# 生成项目
add_executable(mpp_test ${DIR_SRCS})