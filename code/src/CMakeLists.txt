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
include_directories(../src/common)
include_directories(../src/hisi_common)

# 设置链接库路径
link_directories(../../../bin)
# 链接库
link_libraries(_hiae _hiawb _hiawb_natura _hidehaze _hidrc _hiir_auto _hildci)
link_libraries(bcd dnvqe hi_cipher hive_AEC hive_AGC hive_ANR hive_common hive_EQ hive_HPF hive_record hive_RES)
link_libraries(isp ive ivp md mpi securec tde upvqe VoiceEngine)
link_libraries(sns_gc2053 sns_gc2053_forcar sns_imx307 sns_imx307_2l sns_imx327 sns_imx327_2l sns_imx335 sns_os05a sns_sc2231 sns_sc2235 sns_sc3235 sns_sc4236)
link_libraries(pthread)

#设置项目生成目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../../bin)

# API测试模块
add_subdirectory(test)
# mipi视频流接入模块
add_subdirectory(PlayMipiCamera)